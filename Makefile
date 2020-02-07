CFLAGS  := -g -std=c99 -Wall -Werror -D_GNU_SOURCE -Werror=format-overflow=0 -Werror=format-truncation=0
LDLIBS  := -lcrypto
PROGS   := fbankld fbankfs fbankd \
           fbankfs-exstack fbankd-exstack \
           fbankfs-nxstack fbankd-nxstack \
           fbankfs-withssp fbankd-withssp

all: $(PROGS)
.PHONY: all

fbankld fbankd fbankfs: %: %.o http.o
fbankfs-withssp fbankd-withssp: %: %.o http-withssp.o

%-nxstack: %
	cp $< $@

%-exstack: %
	cp $< $@
	execstack -s $@

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS) -fno-stack-protector

%-withssp.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

%.bin: %.o
	objcopy -S -O binary -j .text $< $@

.PHONY: clean
clean:
	rm -f *.o *.pyc *.bin $(PROGS)


lab%-handin.tar.gz: clean
	tar cf - `find . -type f | grep -v '^\.*$$' | grep -v '/CVS/' | grep -v '/\.svn/' | grep -v '/\.git/' | grep -v 'lab[0-9].*\.tar\.gz' | grep -v '/submit.token$$'` | gzip > $@
