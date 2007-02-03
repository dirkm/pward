CFLAGS:=-g -std=c99 -Wall
LDFLAGS:=-g -lproc-3.2.6

SOURCES:= pward.c proc_impl.c

pward: ${SOURCES}
	${CC} ${CFLAGS} ${LDFLAGS} -pipe -combine ${SOURCES} -o $@
