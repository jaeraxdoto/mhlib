# makefile for building the qap example
# written by Guenther Raidl and Daniel Wagner

# default location of MHLIB is parent directory
ifndef MHLIB
  MHLIB=..
endif

include $(MHLIB)/makefile.common
CCFLAGS += -L$(MHLIB) -I$(MHLIB)

EXE=sched

HEADERS=

SRCS=sched.C

OBJS=$(SRCS:.C=.o)

DEPS=$(SRCS:.C=.d)

$(EXE): $(OBJS) $(MHLIB)/libmh.a
	$(CPP) $(CCFLAGS) -o $(EXE) $(OBJS) -lmh

all: $(EXE)

clean:
	rm -f $(OBJS) $(DEPS) $(EXE)

sinclude $(DEPS)
