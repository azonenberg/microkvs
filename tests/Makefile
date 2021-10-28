CFLAGS=-g -O2
CXXFLAGS=$(CFLAGS) --std=c++17 -fno-exceptions -fno-rtti \
	-I../
CC=gcc
CXX=g++

all:
	$(CXX) -c ../kvs/*.cpp $(CXXFLAGS)
	$(CXX) -c ../driver/TestStorageBank.cpp $(CXXFLAGS)
	$(CXX) -c *.cpp $(CXXFLAGS)
	$(CXX) *.o -o test $(CXXFLAGS)
