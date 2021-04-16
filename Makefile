CXX = g++
LIBARIES = -lstdc++fs -lSDL2 -DFMT_HEADER_ONLY
CXXFLAGS = -g -std=c++2a -I $(INCLUDEDIR)
BIN = bin/
SOURCEDIR = src/
INCLUDEDIR = include/
OBJECTS = Arm7Tdmi.o util.o Memory.o PPU.o Gamepad.o Timer.o IRQ.o APU.o HandlerArm.o HandlerThumb.o swi.o
VPATH = $(SOURCEDIR)
TESTS = $(SOURCEDIR)tests/tests.cpp $(SOURCEDIR)tests/instruction_tests.cpp $(SOURCEDIR)tests/data_processing_tests.cpp

# Use compiler optimizations
OPT = 0

ifeq (1, $(OPT))
CXXFLAGS += -Ofast
endif

all: discovery

discovery: $(OBJECTS) Discovery.cpp
	$(CXX) $(CXXFLAGS) -o discovery $(SOURCEDIR)Discovery.cpp $(OBJECTS) $(LIBARIES)

test: $(OBJECTS) $(TESTS)
	$(CXX) -o test $(OBJECTS) $(TESTS) $(LIBARIES)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
