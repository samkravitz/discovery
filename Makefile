CC = g++
LIBARIES = -lstdc++fs -lSDL2
CPPFLAGS = -g -I $(INCLUDEDIR) #-O2
BIN = bin/
SOURCEDIR = src/
INCLUDEDIR = include/
OBJECTS = arm_7tdmi.o util.o memory.o gpu.o arm_alu.o thumb_alu.o swi.o
VPATH = $(SOURCEDIR)
TESTS = $(SOURCEDIR)tests/tests.cpp $(SOURCEDIR)tests/instruction_tests.cpp $(SOURCEDIR)tests/data_processing_tests.cpp

all: discovery #mov

discovery: $(OBJECTS) discovery.cpp
	$(CC) $(CPPFLAGS) -o discovery $(SOURCEDIR)discovery.cpp $(OBJECTS) $(LIBARIES)

test: $(OBJECTS) $(TESTS)
	$(CC) -o test $(OBJECTS) $(TESTS) $(LIBARIES)

%.o : %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

mov:
	mv *.o bin/

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
