CC = g++
LIBARIES = -lstdc++fs -l SDL2
CFLAGS = -g 
BIN = bin/
SOURCEDIR = src/
OBJECTS = arm_7tdmi.o util.o memory.o gpu.o lcd_stat.o obj_attr.o arm_alu.o thumb_alu.o swi.o
VPATH = $(SOURCEDIR)
TESTS = $(SOURCEDIR)tests/tests.cpp $(SOURCEDIR)tests/instruction_tests.cpp $(SOURCEDIR)tests/data_processing_tests.cpp

all: discovery #mov

discovery: $(OBJECTS) $(SOURCEDIR)discovery.cpp
	$(CC) $(CFLAGS) -o discovery $(OBJECTS) $(SOURCEDIR)discovery.cpp $(LIBARIES)

test: $(OBJECTS) $(TESTS)
	g++ -o test $(OBJECTS) $(TESTS) $(LIBARIES)

arm_7tdmi.o: $(SOURCEDIR)arm_7tdmi.cpp $(SOURCEDIR)arm_7tdmi.h
	$(CC) $(CFLAGS) -c $(SOURCEDIR)arm_7tdmi.cpp

arm_alu.o: $(SOURCEDIR)arm_alu.cpp $(SOURCEDIR)arm_7tdmi.h
	$(CC) $(CFLAGS) -c $(SOURCEDIR)arm_alu.cpp

thumb_alu.o: $(SOURCEDIR)thumb_alu.cpp $(SOURCEDIR)arm_7tdmi.h
	$(CC) $(CFLAGS) -c $(SOURCEDIR)thumb_alu.cpp

swi.o: $(SOURCEDIR)swi.cpp $(SOURCEDIR)arm_7tdmi.h
	$(CC) $(CFLAGS) -c $(SOURCEDIR)swi.cpp

util.o: $(SOURCEDIR)common/util.h $(SOURCEDIR)common/util.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)common/util.cpp

memory.o: $(SOURCEDIR)memory.h $(SOURCEDIR)memory.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)memory.cpp

gpu.o: $(SOURCEDIR)gpu.h $(SOURCEDIR)gpu.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)gpu.cpp

lcd_stat.o: $(SOURCEDIR)lcd_stat.h $(SOURCEDIR)lcd_stat.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)lcd_stat.cpp

obj_attr.o: $(SOURCEDIR)obj_attr.h $(SOURCEDIR)obj_attr.cpp
	$(CC) $(CFLAGS) -c $(SOURCEDIR)obj_attr.cpp

mov:
	mv *.o bin/

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
