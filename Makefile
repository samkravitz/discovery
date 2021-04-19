CXX = g++
LIBARIES = -lstdc++fs -lSDL2 -DFMT_HEADER_ONLY
CXXFLAGS = -g -std=c++2a -I $(INCLUDEDIR) -I $(BACKUPDIR)
BIN = bin/
SOURCEDIR = src/
INCLUDEDIR = include/
BACKUPDIR = $(INCLUDEDIR)backup
OBJECTS = Arm7Tdmi.o util.o Memory.o PPU.o Gamepad.o Timer.o IRQ.o APU.o HandlerArm.o HandlerThumb.o swi.o Flash.o None.o
LIST=$(addprefix $(BIN), $(OBJECTS))
VPATH = $(SOURCEDIR) $(SOURCEDIR)backup

# Use compiler optimizations
# run `make opt=1`
ifdef opt
CXXFLAGS += -Ofast
endif

all: discovery

discovery: $(LIST) Discovery.cpp
	$(CXX) $(CXXFLAGS) -o discovery $(SOURCEDIR)Discovery.cpp $(LIST) $(LIBARIES)

$(BIN)%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
