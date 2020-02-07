#include "http.h"
#include <sys/param.h>
#ifndef BSD
#include <sys/sendfile.h>
#endif
#include <sys/uio.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int http_read_line(int fd, char *buf, size_t size)
{
    size_t i = 0;

    for (;;)
    {
        int cc = read(fd, &buf[i], 1);
        if (cc <= 0)
            break;

        if (buf[i] == '\r')
        {
            buf[i] = '\0';      /* preskoc */
            continue;
        }

        if (buf[i] == '\n')
        {
            buf[i] = '\0';
            return 0;
        }

        if (i >= size - 1)
        {
            buf[i] = '\0';
            return 0;
        }

        i++;
    }

    return -1;
}

const char *http_request_line(int fd, char *reqpath, char *env, size_t *env_len)
{
    static char buf[8192];      /* POZOR!!! staticke premenne sa nenachadzaju na zasobniku */
    char *sp1, *sp2, *qp, *envp = env;

    if (http_read_line(fd, buf, sizeof(buf)) < 0)
        return "Socket IO error";

    /* Parsuje requesty ako napr. "GET /foo.html HTTP/1.0" */
    sp1 = strchr(buf, ' ');
    if (!sp1)
        return "Cannot parse HTTP request (1)";
    *sp1 = '\0';
    sp1++;
    if (*sp1 != '/')
        return "Bad request path";

    sp2 = strchr(sp1, ' ');
    if (!sp2)
        return "Cannot parse HTTP request (2)";
    *sp2 = '\0';
    sp2++;

    /* !!! HTTP server podporuje iba GET a POST requesty */
    if (strcmp(buf, "GET") && strcmp(buf, "POST"))
        return "Unsupported request (not GET or POST)";

    envp += sprintf(envp, "REQUEST_METHOD=%s", buf) + 1;
    envp += sprintf(envp, "SERVER_PROTOCOL=%s", sp2) + 1;

    /* parsuje query string, napr. "foo.py?user=bob" */
    if ((qp = strchr(sp1, '?')))
    {
        *qp = '\0';
        envp += sprintf(envp, "QUERY_STRING=%s", qp + 1) + 1;
    }

    /* dekoduje URL 'escape' sekvencie v pozadovanej ceste do premennej reqpath */
    url_decode(reqpath, sp1);

    envp += sprintf(envp, "REQUEST_URI=%s", reqpath) + 1;

    envp += sprintf(envp, "SERVER_NAME=zoobar.org") + 1;

    *envp = 0;
    *env_len = envp - env + 1;
    return NULL;
}

const char *http_request_headers(int fd)
{
    static char buf[8192];      /* POZOR!!! staticke premenne sa nenachadzaju na zasobniku */
    int i;
    char value[512];
    char envvar[512];

    /* Parsovanie HTTP hlaviciek */
    for (;;)
    {
        if (http_read_line(fd, buf, sizeof(buf)) < 0)
            return "Socket IO error";

        if (buf[0] == '\0')     /* koniec hlaviciek */
            break;

        /* Parsuje riadky typu "Cookie: foo bar" */
        char *sp = strchr(buf, ' ');
        if (!sp)
            return "Header parse error (1)";
        *sp = '\0';
        sp++;

        /* Odstrani ciarku, kontrola ci tam aj naozaj je */
        if (strlen(buf) == 0)
            return "Header parse error (2)";

        char *colon = &buf[strlen(buf) - 1];
        if (*colon != ':')
            return "Header parse error (3)";
        *colon = '\0';
        
        /* Nastavi na nazvy hlaviciek na velke pismena a pomlcky nahradi podciarkovnikmi */
        for (i = 0; i < strlen(buf); i++) {
            buf[i] = toupper(buf[i]);
            if (buf[i] == '-')
                buf[i] = '_';
        }

        /* Dekoduje 'escape' sekvencie v premennej value */
        url_decode(value, sp);

        /* Ulozi hlavicku v premennej prostredia pre kod aplikacie */
        /* Niektore specialne hlavicky nepouzivaju HTTP_ prefix */
        if (strcmp(buf, "CONTENT_TYPE") != 0 &&
            strcmp(buf, "CONTENT_LENGTH") != 0) {
            sprintf(envvar, "HTTP_%s", buf);
            setenv(envvar, value, 1);
        } else {
            setenv(buf, value, 1);
        }
    }

    return 0;
}

void http_err(int fd, int code, char *fmt, ...)
{
    fdprintf(fd, "HTTP/1.0 %d Error\r\n", code);
    fdprintf(fd, "Content-Type: text/html\r\n");
    fdprintf(fd, "\r\n");
    fdprintf(fd, "<H1>An error occurred</H1>\r\n");

    char *msg = 0;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&msg, fmt, ap);
    va_end(ap);

    fdprintf(fd, "%s\n", msg);

    close(fd);
    warnx("[%d] Request failed: %s", getpid(), msg);
    free(msg);
}

/* rozdel cestu na nazov skriptu a info o danej ceste */
void split_path(char *pn)
{
    struct stat st;
    char *slash = NULL;

    for (;;) {
         /*
          * Prestane dalej hladat ak najdeme subor na pozicii prefixu,
          * alebo ak nenastane neocakavana chyba
          */
        int r = stat(pn, &st);
        if (r < 0) {
            if (errno != ENOTDIR && errno != ENOENT)
                break;
        } else {
            if (S_ISREG(st.st_mode))
                break;
        }

        /* Nastavi posledne '/' v premennej pn na NULL a skontroluje ci to pomohlo.
           Ak ano, zvysok retazca nastavime do PATH_INFO.
           Ak nie, iterujeme dalej a nastavime predchadzajuce '/' na NULL atd.
        */
        if (slash)
            *slash = '/';
        else
            slash = pn + strlen(pn);

        while (--slash > pn) {
            if (*slash == '/') {
                *slash = '\0';
                break;
            }
        }

        if (slash == pn) {
            slash = NULL;
            break;
        }
    }

    if (slash) {
        *slash = '/';
        setenv("PATH_INFO", slash, 1);
        *slash = '\0';
    }

    setenv("SCRIPT_NAME", pn + strlen(getenv("DOCUMENT_ROOT")), 1);
    setenv("SCRIPT_FILENAME", pn, 1);
}

static int cgi_uid = -1;
static int cgi_gid = -1;

void
http_set_executable_uid_gid(int uid, int gid)
{
    cgi_uid = uid;
    cgi_gid = gid;
}

static int
valid_cgi_script(struct stat *st)
{
    if (!S_ISREG(st->st_mode))
        return 0;

    if (!(st->st_mode & S_IXUSR))
        return 0;

    if (cgi_uid >= 0 && cgi_gid >= 0) {
        if (st->st_uid != cgi_uid || st->st_gid != cgi_gid)
            return 0;
    }

    return 1;
}

void http_serve(int fd, const char *name)
{
    void (*handler)(int, const char *) = http_serve_none;
    char pn[1024];
    struct stat st;

    getcwd(pn, sizeof(pn));
    setenv("DOCUMENT_ROOT", pn, 1);

    strcat(pn, name);
    split_path(pn);

    if (!stat(pn, &st))
    {
        /* spustitelny bit nastaveny -> mozeme spustit ako CGI skript */
        if (valid_cgi_script(&st))
            handler = http_serve_executable;
        else if (S_ISDIR(st.st_mode))
            handler = http_serve_directory;
        else
            handler = http_serve_file;
    }

    handler(fd, pn);
}

void http_serve_none(int fd, const char *pn)
{
    http_err(fd, 404, "File does not exist: %s", pn);
}

void http_serve_file(int fd, const char *pn)
{
    int filefd;
    off_t len = 0;

    if (getenv("PATH_INFO")) {
        /* o PATH_INFO sa pokusaj len pri dynamickych zdrojoch */
        char buf[1024];
        snprintf(buf, 1024, "%s%s", pn, getenv("PATH_INFO"));
        http_serve_none(fd, buf);
        return;
    }

    if ((filefd = open(pn, O_RDONLY)) < 0)
        return http_err(fd, 500, "open %s: %s", pn, strerror(errno));

    const char *ext = strrchr(pn, '.');
    const char *mimetype = "text/html";
    if (ext && !strcmp(ext, ".css"))
        mimetype = "text/css";
    if (ext && !strcmp(ext, ".jpg"))
        mimetype = "image/jpeg";

    fdprintf(fd, "HTTP/1.0 200 OK\r\n");
    fdprintf(fd, "Content-Type: %s\r\n", mimetype);
    fdprintf(fd, "\r\n");

#ifndef BSD
    struct stat st;
    if (!fstat(filefd, &st))
        len = st.st_size;
    if (sendfile(fd, filefd, 0, len) < 0)
#else
    if (sendfile(filefd, fd, 0, &len, 0, 0) < 0)
#endif
        err(1, "sendfile");
    close(filefd);
}

void dir_join(char *dst, const char *dirname, const char *filename) {
    strcpy(dst, dirname);
    if (dst[strlen(dst) - 1] != '/')
        strcat(dst, "/");
    strcat(dst, filename);
}

void http_serve_directory(int fd, const char *pn) {
    /* pre priecinky, pouzi len index.html alebo podobny subor v tom istom priecinku */
    static const char * const indices[] = {"index.html", "index.php", "index.cgi", NULL};
    char name[1024];
    struct stat st;
    int i;

    for (i = 0; indices[i]; i++) {
        dir_join(name, pn, indices[i]);
        if (stat(name, &st) == 0 && S_ISREG(st.st_mode)) {
            dir_join(name, getenv("SCRIPT_NAME"), indices[i]);
            break;
        }
    }

    if (indices[i] == NULL) {
        http_err(fd, 403, "No index file in %s", pn);
        return;
    }

    http_serve(fd, name);
}

void http_serve_executable(int fd, const char *pn)
{
    char buf[1024], headers[4096], *pheaders = headers;
    int pipefd[2], statusprinted = 0, ret, headerslen = 4096;

    pipe(pipefd);
    switch (fork()) {
    case -1:
        http_err(fd, 500, "fork: %s", strerror(errno));
        return;
    case 0:
        signal(SIGPIPE, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        dup2(fd, 0);
        close(fd);
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        close(pipefd[1]);
        execl(pn, pn, NULL);
        http_err(1, 500, "execl %s: %s", pn, strerror(errno));
        exit(1);
    default:
        close(pipefd[1]);
        while (1) {
            if (http_read_line(pipefd[0], buf, 1024) < 0) {
                http_err(fd, 500, "Premature end of script headers");
                close(pipefd[0]);
                return;
            }

            if (!*buf)
                break;

            if (!statusprinted && strncasecmp("Status: ", buf, 8) == 0) {
                fdprintf(fd, "HTTP/1.0 %s\r\n%s", buf + 8, headers);
                statusprinted = 1;
            } else if (statusprinted) {
                fdprintf(fd, "%s\r\n", buf);
            } else {
                ret = snprintf(pheaders, headerslen, "%s\r\n", buf);
                pheaders += ret;
                headerslen -= ret;
                if (headerslen == 0) {
                    http_err(fd, 500, "Too many script headers");
                    close(pipefd[0]);
                    return;
                }
            }
        }

        if (statusprinted)
            fdprintf(fd, "\r\n");
        else
            fdprintf(fd, "HTTP/1.0 200 OK\r\n%s\r\n", headers);

        while ((ret = read(pipefd[0], buf, 1024)) > 0) {
            write(fd, buf, ret);
        }

        close(fd);
        close(pipefd[0]);
    }
}

void url_decode(char *dst, const char *src)
{
    for (;;)
    {
        if (src[0] == '%' && src[1] && src[2])
        {
            char hexbuf[3];
            hexbuf[0] = src[1];
            hexbuf[1] = src[2];
            hexbuf[2] = '\0';

            *dst = strtol(&hexbuf[0], 0, 16);
            src += 3;
        }
        else if (src[0] == '+')
        {
            *dst = ' ';
            src++;
        }
        else
        {
            *dst = *src;
            src++;

            if (*dst == '\0')
                break;
        }

        dst++;
    }
}

void env_deserialize(const char *env, size_t len)
{
    for (;;)
    {
        char *p = strchr(env, '=');
        if (p == 0 || p - env > len)
            break;
        *p++ = 0;
        setenv(env, p, 1);
        p += strlen(p)+1;
        len -= (p - env);
        env = p;
    }
    setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
    setenv("REDIRECT_STATUS", "200", 1);
}

void fdprintf(int fd, char *fmt, ...)
{
    char *s = 0;

    va_list ap;
    va_start(ap, fmt);
    vasprintf(&s, fmt, ap);
    va_end(ap);

    write(fd, s, strlen(s));
    free(s);
}

ssize_t sendfd(int socket, const void *buffer, size_t length, int fd)
{
    struct iovec iov = {(void *)buffer, length};
    char buf[CMSG_LEN(sizeof(int))];
    struct cmsghdr *cmsg = (struct cmsghdr *)buf;
    ssize_t r;
    cmsg->cmsg_len = sizeof(buf);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *((int *)CMSG_DATA(cmsg)) = fd;
    struct msghdr msg = {0};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg->cmsg_len;
    r = sendmsg(socket, &msg, 0);
    if (r < 0)
        warn("sendmsg");
    return r;
}

ssize_t recvfd(int socket, void *buffer, size_t length, int *fd)
{
    struct iovec iov = {buffer, length};
    char buf[CMSG_LEN(sizeof(int))];
    struct cmsghdr *cmsg = (struct cmsghdr *)buf;
    ssize_t r;
    cmsg->cmsg_len = sizeof(buf);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    struct msghdr msg = {0};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg->cmsg_len;
again:
    r = recvmsg(socket, &msg, 0);
    if (r < 0 && errno == EINTR)
        goto again;
    if (r < 0)
        warn("recvmsg");
    else
        *fd = *((int*)CMSG_DATA(cmsg));
    return r;
}
