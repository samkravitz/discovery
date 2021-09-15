CXX = g++
LIBARIES = -lSDL2 -DFMT_HEADER_ONLY -lstdc++fs
CXXFLAGS = -g -std=gnu++2a -I $(INCLUDEDIR) -I $(BACKUPDIR)
BIN = bin/
SOURCEDIR = src/
INCLUDEDIR = include/
BACKUPDIR = $(INCLUDEDIR)backup
OBJECTS = Discovery.o Arm7Tdmi.o util.o Memory.o PPU.o Gamepad.o Timer.o IRQ.o APU.o ArmISA.o ThumbISA.o swi.o Flash.o None.o SRAM.o config.o Scheduler.o
LIST=$(addprefix $(BIN), $(OBJECTS))
VPATH = $(SOURCEDIR) $(SOURCEDIR)backup

# Use compiler optimizations
# run `make opt=1`
ifdef opt
CXXFLAGS += -Ofast
endif

all: discovery

discovery: $(LIST) main.cpp
	$(CXX) $(CXXFLAGS) -o discovery $(SOURCEDIR)main.cpp $(LIST) $(LIBARIES)

$(BIN)%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
