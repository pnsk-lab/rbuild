# $Id$

PLATFORM = generic
PWD = `pwd`

FLAGS = PWD=$(PWD) PLATFORM=$(PLATFORM)

.PHONY: all ./Common ./Server ./Client clean

all: ./Common ./Server ./Client

./Common::
	$(MAKE) -C $@ $(FLAGS)

./Server:: ./Common
	$(MAKE) -C $@ $(FLAGS)

./Client:: ./Common
	$(MAKE) -C $@ $(FLAGS)

clean:
	$(MAKE) -C ./Common clean
	$(MAKE) -C ./Server clean
	$(MAKE) -C ./Client clean
