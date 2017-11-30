AUTOMAKE_OPTIONS=foreign  

check_PROGRAMS=rpc_test
rpc_test_SOURCES=rpc_main.cpp
rpc_test_CXXFLAGS=-I../../include -m64 -std=c++11
rpc_test_LDFLAGS=-L../../.libs -leverest  -lboost_system

TESTS=$(check_PROGRAMS)