AUTOMAKE_OPTIONS=foreign  

check_PROGRAMS=test_sobj
test_sobj_SOURCES=sobjtest.cpp
test_sobj_CXXFLAGS=-I../../include -m64 -std=c++11 -g
test_sobj_LDFLAGS=-L../../.libs -leverest_core

TESTS=$(check_PROGRAMS)