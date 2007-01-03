CFLAGS:=-g -std=c99 -Wall
LDFLAGS:=-g -lproc-3.2.6

OBJECTS:= pward.o proc_impl.o acct_impl.o

pward: ${OBJECTS}

