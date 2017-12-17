# Main makefile for building the mh library
# vim: filetype=make

include makefile.common

LIB=libmh.a

HEADERS := $(wildcard *.h)

SRCS := $(wildcard *.C)

OBJS=$(SRCS:.C=.o)
DEPS=$(SRCS:.C=.d)

SUBDIRS=demo-*

.PHONY: all clean doc test $(SUBDIRS)

default: $(LIB) $(SUBDIRS)

$(SUBDIRS): $(LIB)
	$(MAKE) -C $@

$(LIB): $(OBJS)
	$(AR) -rsv $(LIB) $(OBJS)

all: $(LIB) $(SUBDIRS)
	for dir in ${SUBDIRS} ; do ( ${MAKE} -C $$dir all ) ; done

clean:
	rm -f $(OBJS) $(DEPS) $(LIB); \
	for dir in ${SUBDIRS} ; do ( ${MAKE} -C $$dir clean ) ; done
	rm -rf doxy

doc: $(HEADERS)
	doxygen doxy.cfg

test:
	./test-mhlib.sh

sinclude $(DEPS)
