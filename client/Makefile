INCLUDE_DIRS=
LIB_DIRS=
CC 	:= $(CROSS_COMPILE)gcc

CDEFS=
CFLAGS	:= -g -Werror -Wall $(INCLUDE_DIRS) $(CDEFS)
LIBS=  
LDFLAGS := -pthread -lrt

HFILES=
CFILES=aesdsocket_client.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:aesdsocket_client

clean:
	-rm -f *.o *.d *.elf aesdsocket_client
	-rm -f aesdsocket_client

aesdsocket_client:aesdsocket_client.c
	$(CC) aesdsocket_client.c $(CFLAGS) -o $@ $(LDFLAGS)

depend:

.c.o: $(SRCS)
	$(CC) $(CFLAGS) -c $<