CC = g++
LIBARIES = -lstdc++fs -lSDL2 -DFMT_HEADER_ONLY
CPPFLAGS = -g -I $(INCLUDEDIR) -O2 -std=c++2a
BIN = bin/
SOURCEDIR = src/
INCLUDEDIR = include/
OBJECTS = Arm7Tdmi.o Util.o Memory.o PPU.o Gamepad.o # HandlerArm.o HandlerThumb.o swi.o
VPATH = $(SOURCEDIR)
TESTS = $(SOURCEDIR)tests/tests.cpp $(SOURCEDIR)tests/instruction_tests.cpp $(SOURCEDIR)tests/data_processing_tests.cpp

all: discovery #mov

discovery: $(OBJECTS) Discovery.cpp
	$(CC) $(CPPFLAGS) -o discovery $(SOURCEDIR)Discovery.cpp $(OBJECTS) $(LIBARIES)

test: $(OBJECTS) $(TESTS)
	$(CC) -o test $(OBJECTS) $(TESTS) $(LIBARIES)

%.o : %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

mov:
	mv *.o bin/

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
	