cmake_minimum_required(VERSION 3.12)
project(mhlib)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_RELWITHASSERT "-O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

#init DVERSION variable with current git commit hash
execute_process(COMMAND git describe --abbrev=4 --dirty --always --tags OUTPUT_VARIABLE DVERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

#compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -DVERSION=\\\"${DVERSION}\\\"")

#define library folders
#set(MHLIB_DIR "../../mhlib")

#include general
#include_directories(${MHLIB_DIR})
include_directories(..)

#linking directories
#link_directories(${MHLIB_DIR})


add_executable(qap
        qap.C
        qapinstance.h
        qapinstance.C
        qapfeature.h
        qapfeature.C
        qapsol.h
        qapsol.C
        qaptabuattribute.h
        qaptabuattribute.C)

target_link_libraries(qap mh)

