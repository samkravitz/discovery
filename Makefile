CC = g++
LIBARIES = -lstdc++fs -lSDL2
CFLAGS = -g #-O2
BIN = bin/
SOURCEDIR = src/
OBJECTS = arm_7tdmi.o util.o memory.o gpu.o lcd_stat.o obj_attr.o arm_alu.o thumb_alu.o swi.o
VPATH = $(SOURCEDIR)
TESTS = $(SOURCEDIR)tests/tests.cpp $(SOURCEDIR)tests/instruction_tests.cpp $(SOURCEDIR)tests/data_processing_tests.cpp

all: discovery #mov

discovery: $(OBJECTS) discovery.cpp
	$(CC) $(CFLAGS) -o discovery $(SOURCEDIR)discovery.cpp $(OBJECTS) $(LIBARIES)

test: $(OBJECTS) $(TESTS)
	g++ -o test $(OBJECTS) $(TESTS) $(LIBARIES)

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

mov:
	mv *.o bin/

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
