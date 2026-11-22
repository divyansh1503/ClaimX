CXX      = g++
CC       = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude -Ic_ds
CFLAGS   = -std=c11 -Wall -Wextra -g -Ic_ds

dslib: src/main.cpp build/stack.o build/hashindex.o $(wildcard include/*.h)
	$(CXX) $(CXXFLAGS) src/main.cpp build/stack.o build/hashindex.o -o dslib

build/stack.o: c_ds/stack.c c_ds/stack.h
	mkdir -p build
	$(CC) $(CFLAGS) -c c_ds/stack.c -o build/stack.o

build/hashindex.o: c_ds/hashindex.c c_ds/hashindex.h
	mkdir -p build
	$(CC) $(CFLAGS) -c c_ds/hashindex.c -o build/hashindex.o

clean:
	rm -rf build dslib
