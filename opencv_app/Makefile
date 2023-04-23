# the ?= is not compatible in all the make versions so making it cross compatible by using := instead
INCLUDE_DIRS = -I /usr/include/opencv4/opencv2
#INCLUDE_DIRS=
LIB_DIRS = 
CROSS_COMPILE =
ifeq ($(CC),)
	CC = $(CROSS_COMPILE)g++
endif
#CC 	:= $(CROSS_COMPILE)g++
ifeq ($(CFLAGS),)
	CFLAGS:= -O0 -g -I /usr/include/opencv4/opencv2 $(CDEFS)
endif
CDEFS=

LIBS= -L/usr/lib -lopencv_core -lopencv_flann -lopencv_video -lrt -lstdc++fs -lpthread -lstdc++
#LIBS=
HFILES= 
CFILES= aesd_opencv.cpp

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.cpp=.o}


all:	aesd_opencv

clean:
	-rm -f *.o *.d
	-rm -f aesd_opencv

aesd_opencv: aesd_opencv.cpp
	$(CC) aesd_opencv.cpp $(CFLAGS) -o $@ `pkg-config --libs opencv4` $(LIBS)

depend:

.cpp.o: $(SRCS)
	$(CC) $(CFLAGS) -c $<
