AUTOMAKE_OPTIONS=foreign subdir-objects
#SUBDIRS=. samples
SUBDIRS=. test
ACLOCAL_AMFLAGS= -I m4

lib_LTLIBRARIES= libeverest_core.la
libeverest_core_la_SOURCES=src/SObject.cpp
libeverest_core_la_CPPFLAGS=-m64 -std=c++11 -Iinclude -g
libeverest_core_la_LDFLAGS=-fPIC

