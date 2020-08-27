CC = g++
LIBARIES = -lstdc++fs -l SDL2
CFLAGS = -g 
BIN = bin/
SOURCEDIR = src/
OBJECTS = arm_7tdmi.o util.o memory.o gpu.o arm_alu.o
VPATH = $(SOURCEDIR)
TESTS = $(SOURCEDIR)tests/tests.cpp $(SOURCEDIR)tests/instruction_tests.cpp $(SOURCEDIR)tests/data_processing_tests.cpp

all: discovery #mov

discovery: $(OBJECTS) $(SOURCEDIR)discovery.cpp
	$(CC) $(CFLAGS) -o discovery $(OBJECTS) $(SOURCEDIR)discovery.cpp $(LIBARIES)

test: $(OBJECTS) $(TESTS)
	g++ -o test $(OBJECTS) $(TESTS) $(LIBARIES)

arm_7tdmi.o: $(SOURCEDIR)arm_7tdmi.h $(SOURCEDIR)arm_7tdmi.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)arm_alu.cpp $(SOURCEDIR)arm_7tdmi.cpp

util.o: $(SOURCEDIR)common/util.h $(SOURCEDIR)common/util.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)common/util.cpp

memory.o: $(SOURCEDIR)memory.h $(SOURCEDIR)memory.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)memory.cpp

gpu.o: $(SOURCEDIR)gpu.h $(SOURCEDIR)gpu.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)gpu.cpp

mov:
	mv *.o bin/

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
