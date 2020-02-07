#pragma once

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stddef.h>

/** Precita jeden riadok HTTP requestu, napr. "GET /xxx HTTP/1.0".
 *  \return Chybova hlaska alebo NULL ak prebehlo vsetko v poriadku.
 */
const char *http_request_line(int fd, char *reqpath, char *env, size_t *env_len);

/** Precita vsetky hlavicky HTTP requestu.
 *  \return Chybova hlaska alebo NULL ak prebehlo vsetko v poriadku.
 */
const char *http_request_headers(int fd);

/** Odosle HTTP chybovu spravu. */
void http_err(int fd, int code, char *fmt, ...);

/** Funkcia na generovanie responsov na HTTP requesty. */
void http_serve(int fd, const char *);

void http_serve_none(int fd, const char *);

void http_serve_file(int fd, const char *);

void http_serve_directory(int fd, const char *);

void http_serve_executable(int fd, const char *);

void http_set_executable_uid_gid(int uid, int gid);

/** URL dekoder. */
void url_decode(char *dst, const char *src);

/** Vyparsuje retazce specifikujuce nastavenia premennych prostredia a nastavi ich. */
void env_deserialize(const char *env, size_t len);

void fdprintf(int fd, char *fmt, ...);

ssize_t sendfd(int socket, const void *buffer, size_t length, int fd);

ssize_t recvfd(int socket, void *buffer, size_t length, int *fd);
