# makefile for building the mh library
# written by Guenther Raidl and Daniel Wagner

include makefile.common

LIB=libmh.a

HEADERS=mh_advbase.h mh_base.h mh_binstringchrom.h mh_solution.h mh_fdc.h \
	mh_feature.h mh_genea.h mh_grasp.h mh_guidedls.h mh_hash.h mh_interfaces.h \
	mh_island.h mh_localsearch.h mh_log.h mh_lsbase.h mh_move.h mh_param.h \
	mh_permchrom.h mh_popbase.h mh_pop.h mh_popsupp.h mh_random.h \
	mh_simanneal.h mh_ssea.h mh_stringchrom.h mh_stringchrom_impl.h \
	mh_subpop.h mh_tabuattribute.h mh_tabulist.h mh_tabusearch.h mh_util.h \
	mh_vns.h mh_vnd.h mh_allalgs.h

SRCS=mh_advbase.C mh_base.C mh_binstringchrom.C mh_solution.C mh_fdc.C \
	mh_feature.C mh_genea.C mh_grasp.C mh_guidedls.C mh_hash.C mh_interfaces.C \
	mh_island.C mh_localsearch.C mh_log.C mh_lsbase.C mh_move.C mh_param.C \
	mh_permchrom.C mh_popbase.C mh_pop.C mh_popsupp.C mh_random.C \
	mh_simanneal.C mh_ssea.C mh_stringchrom.C mh_subpop.C \
	mh_tabuattribute.C mh_tabulist.C mh_tabusearch.C mh_util.C \
	mh_vns.C mh_vnd.C mh_allalgs.C

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

doc: $(HEADERS)
	doxygen doxy.cfg

sinclude $(DEPS)
