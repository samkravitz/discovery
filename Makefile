CXX = g++
LIBS = -lstdc++fs -lSDL2 -lfmt
CXXFLAGS = -g -std=c++2a -I $(INCLUDEDIR) -I $(BACKUPDIR)
BIN = bin
SOURCEDIR = src
INCLUDEDIR = include
BACKUPDIR = $(INCLUDEDIR)/backup
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

LIST = $(addprefix $(BIN)/, $(OBJ))
VPATH = $(SOURCEDIR) $(SOURCEDIR)/backup

# Use compiler optimizations
# run `make opt=1`
ifdef opt
CXXFLAGS += -Ofast
endif

# Optionally compile with WxWidgets gui
# run `make gui=1`
ifdef gui

OBJ += \
	App.o \
	Frame.o \
	Panel.o \

VPATH += $(SOURCEDIR)/platform/wx
INCLUDE += $(SOURCEDIR)/platform/wx

MAIN = $(SOURCEDIR)/platform/wx/main.cpp
CXXFLAGS += `wx-config --cxxflags`
LIBS += `wx-config --libs`
else
MAIN = $(SOURCEDIR)/platform/sdl/main.cpp
endif


all: discovery
discovery: $(LIST) $(MAIN)
	$(CXX) $(CXXFLAGS) -o discovery $(MAIN) $(LIST) $(LIBS)

$(BIN)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
