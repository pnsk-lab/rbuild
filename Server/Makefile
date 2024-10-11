# $Id$

include $(PWD)/Platform/$(PLATFORM).mk

.PHONY: all clean
.SUFFIXES: .c .o

OBJS = main.o
OBJS += ../Common/common.a

all: rbuild-server$(EXEC)

rbuild-server$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.exe rbuild-server
