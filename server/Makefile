INCLUDE_DIRS=
LIB_DIRS=
CC 	:= $(CROSS_COMPILE)gcc

CDEFS=
CFLAGS	:= -g -Werror -Wall $(INCLUDE_DIRS) $(CDEFS)
LIBS=  
LDFLAGS := -pthread -lrt

HFILES=
CFILES=aesdsocket_server.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:aesdsocket_server

clean:
	-rm -f *.o *.d *.elf aesdsocket_server
	-rm -f aesdsocket_server

aesdsocket_server:aesdsocket_server.c
	$(CC) aesdsocket_server.c $(CFLAGS) -o $@ $(LDFLAGS)

depend:

.c.o: $(SRCS)
	$(CC) $(CFLAGS) -c $<