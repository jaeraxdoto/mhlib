cmake_minimum_required(VERSION 3.12)
project(mhlib)

set(CMAKE_CXX_STANDARD 14)

#init DVERSION variable with current git commit hash
execute_process(COMMAND git describe --abbrev=4 --dirty --always --tags OUTPUT_VARIABLE DVERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

#compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -DVERSION=\\\"${DVERSION}\\\"")

include_directories(.)

add_subdirectory(demo-maxsat)
add_subdirectory(demo-onemax)
add_subdirectory(demo-qap)
add_subdirectory(demo-recsched)
add_subdirectory(demo-sched)

add_library(mh
        mh_advbase.C
        mh_advbase.h
        mh_allalgs.C
        mh_allalgs.h
        mh_base.h
        mh_binstringsol.C
        mh_binstringsol.h
        mh_c11threads.h
        mh_eaadvbase.C
        mh_eaadvbase.h
        mh_fdc.C
        mh_fdc.h
        mh_feature.C
        mh_feature.h
        mh_gaopsprov.C
        mh_gaopsprov.h
        mh_genea.C
        mh_genea.h
        mh_grasp.C
        mh_grasp.h
        mh_guidedls.C
        mh_guidedls.h
        mh_gvns.C
        mh_gvns.h
        mh_interfaces.C
        mh_interfaces.h
        mh_island.C
        mh_island.h
        mh_localsearch.C
        mh_localsearch.h
        mh_log.C
        mh_log.h
        mh_lsbase.C
        mh_lsbase.h
        mh_nhmove.h
        mh_param.C
        mh_param.h
        mh_parscheduler.C
        mh_parscheduler.h
        mh_pbig.C
        mh_pbig.h
        mh_permsol.C
        mh_permsol.h
        mh_pop.C
        mh_pop.h
        mh_popbase.C
        mh_popbase.h
        mh_popsupp.C
        mh_popsupp.h
        mh_random.C
        mh_random.h
        mh_schedmeth.h
        mh_scheduler.C
        mh_scheduler.h
        mh_simanneal.C
        mh_simanneal.h
        mh_solution.C
        mh_solution.h
        mh_ssea.C
        mh_ssea.h
        mh_stringsol.C
        mh_stringsol.h
        mh_subpop.C
        mh_subpop.h
        mh_tabuattribute.h
        mh_tabulist.C
        mh_tabulist.h
        mh_tabusearch.C
        mh_tabusearch.h
        mh_util.C
        mh_util.h
        mh_vnd.C
        mh_vnd.h
        mh_vns.C
        mh_vns.h)


