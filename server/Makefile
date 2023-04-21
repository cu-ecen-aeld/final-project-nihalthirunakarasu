INCLUDE_DIRS=
LIB_DIRS=
CC 	?= $(CROSS_COMPILE)gcc

CDEFS=
CFLAGS	?= -g -Werror -Wall $(INCLUDE_DIRS) $(CDEFS)
LIBS=  
LDFLAGS ?= -pthread -lrt

HFILES=
CFILES=aesdsocket.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:aesdsocket

clean:
	-rm -f *.o *.d *.elf aesdsocket
	-rm -f aesdsocket

aesdsocket:aesdsocket.c
	$(CC) aesdsocket.c $(CFLAGS) -o $@ $(LDFLAGS)

depend:

.c.o: $(SRCS)
	$(CC) $(CFLAGS) -c $<