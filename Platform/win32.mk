# $Id$

CC = i686-w64-mingw32-gcc
AR = i686-w64-mingw32-ar
CFLAGS = -I $(PWD)/Common -DHAS_PLAIN_AUTH -DHAS_NONE_AUTH
LDFLAGS =
LIBS = -lws2_32
EXEC = .exe
