# makefile for building the mh library
# written by Guenther Raidl and Daniel Wagner

include makefile.common

LIB=libmh.a

HEADERS := $(wildcard *.h)

SRCS := $(wildcard *.C)

OBJS=$(SRCS:.C=.o)
DEPS=$(SRCS:.C=.d)

SUBDIRS=demo-onemax demo-qap

.PHONY: all clean doc $(SUBDIRS)

default: $(LIB) $(SUBDIRS)

$(SUBDIRS): $(LIB)
	$(MAKE) -C $@

$(LIB): $(OBJS)
	ar -rv $(LIB) $(OBJS)

all: $(LIB) $(SUBDIRS)
	for dir in ${SUBDIRS} ; do ( ${MAKE} -C $$dir all ) ; done

clean:
	rm -f $(OBJS) $(DEPS) $(LIB); \
	for dir in ${SUBDIRS} ; do ( ${MAKE} -C $$dir clean ) ; done
	rm -rf doxy

doc: $(HEADERS)
	doxygen doxy.cfg

sinclude $(DEPS)
