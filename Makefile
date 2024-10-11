# $Id$

PLATFORM = generic
PWD = `pwd`

FLAGS = PWD=$(PWD) PLATFORM=$(PLATFORM)

.PHONY: all ./Common ./Server ./Client clean format

all: ./Common ./Server ./Client

./Common::
	$(MAKE) -C $@ $(FLAGS)

./Server:: ./Common
	$(MAKE) -C $@ $(FLAGS)

./Client:: ./Common
	$(MAKE) -C $@ $(FLAGS)

clean:
	$(MAKE) -C ./Common clean $(FLAGS)
	$(MAKE) -C ./Server clean $(FLAGS)
	$(MAKE) -C ./Client clean $(FLAGS)

format:
	clang-format --verbose -i `find . -name "*.c" -or -name "*.h"`
