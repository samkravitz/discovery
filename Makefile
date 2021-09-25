CXX = g++
LIBS = -lstdc++fs -lSDL2 -lfmt
CXXFLAGS = -g -std=c++2a -I $(INCLUDE) -I $(BACKUPDIR)
BIN = bin
SOURCE = src
INCLUDE = include
BACKUPDIR = $(INCLUDE)/backup
OBJ = \
	APU.o \
	Arm7Tdmi.o \
	ArmISA.o \
	config.o \
	Discovery.o \
	Flash.o \
	Gamepad.o \
	IRQ.o \
	Memory.o \
	None.o \
	PPU.o \
	Scheduler.o \
	SRAM.o \
	swi.o \
	ThumbISA.o \
	Timer.o \
	util.o \
	Watcher.o \

LIST = $(addprefix $(BIN)/, $(OBJ))
VPATH = $(SOURCE) $(SOURCE)/backup

# Use compiler optimizations
# run `make opt=1`
ifdef opt
CXXFLAGS += -Ofast
endif

all: discovery
discovery: $(LIST) main.cpp
	$(CXX) $(CXXFLAGS) -o discovery $(SOURCE)/main.cpp $(LIST) $(LIBS)

$(BIN)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
