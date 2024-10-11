# $Id$

CC = x86_64-w64-mingw32-gcc
AR = x86_64-w64-mingw32-ar
CFLAGS = -I $(PWD)/Common -DHAS_PLAIN_AUTH -DHAS_NONE_AUTH
LDFLAGS =
LIBS = -lws2_32
EXEC = .exe
