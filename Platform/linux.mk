# $Id: netbsd.mk 7 2024-10-11 16:38:06Z nishi $

CC = cc
AR = ar
CFLAGS = -I $(PWD)/Common -DHAS_PLAIN_AUTH -DHAS_NONE_AUTH -DHAS_PAM_AUTH -DHAS_CRYPT_AUTH
LDFLAGS =
LIBS = -lcrypt -lpam
EXEC =
