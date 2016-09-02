# makefile for building thid application

# default location of MHLIB is parent directory
#ifndef MHLIB	# remove comments for applications outside mhlib directory
  MHLIB=..
#endif

include $(MHLIB)/makefile.common
CCFLAGS += -L$(MHLIB) -I$(MHLIB)

EXE=maxsat

HEADERS=

SRCS=maxsat.C maxsat_inst.C maxsat_sol.C

OBJS=$(SRCS:.C=.o)

DEPS=$(SRCS:.C=.d)

$(EXE): $(OBJS) $(MHLIB)/libmh.a
	$(CPP) $(CCFLAGS) -o $(EXE) $(OBJS) -lmh

all: $(EXE)

clean:
	rm -f $(OBJS) $(DEPS) $(EXE)

sinclude $(DEPS)
