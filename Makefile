PREFIX  := /usr/local
CC      := cc
CFLAGS  := -pedantic -Wall -O2
LDFLAGS := -lX11

# FreeBSD (uncomment)
#LDFLAGS += -L/usr/local/lib -I/usr/local/include
# OpenBSD (uncomment)
#LDFLAGS += -L/usr/X11R6/lib -I/usr/X11R6/include

all: options scells

options:
	@echo "scells build options:"
	@echo "CC      = ${CC}"
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"

scells: scells.c config.def.h config
	${CC} -o scells scells.c ${CFLAGS} ${LDFLAGS}

config:
	cp config.def.h $@.h

clean:
	rm -f *.o *.gch scells

install: scells
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f scells ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/scells

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/scells

.PHONY: all options clean install uninstall
