# $Id$

PLATFORM = generic
PWD = `pwd`

FLAGS = PWD=$(PWD) PLATFORM=$(PLATFORM)

.PHONY: all get-version ./Common ./Server ./Client clean format

all: ./Common ./Server ./Client

get-version:
	@grep "define RBUILD_VERSION" config.h | sed 's/#define RBUILD_VERSION //' | sed 's/"//g'

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
