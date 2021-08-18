CXX = g++
LIBARIES = -lstdc++fs -lSDL2 -lfmt -lGL -lGLEW
CXXFLAGS = -g -std=c++2a -I $(INCLUDEDIR) -I $(BACKUPDIR) -I $(IMGUI_DIR)
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

LIST = $(addprefix $(BIN)/, $(OBJ) $(IMGUI_OBJECTS))
VPATH = $(SOURCEDIR) $(SOURCEDIR)/backup $(IMGUI_DIR) $(IMGUI_DIR)/backends

# Add ImGui files to compile
IMGUI_DIR = third_party/imgui
IMGUI_OBJECTS = imgui.o imgui_draw.o imgui_widgets.o imgui_demo.o imgui_tables.o imgui_impl_sdl.o imgui_impl_opengl3.o
CXXFLAGS += -I -DIMGUI_IMPL_OPENGL_LOADER_GLEW

# Use compiler optimizations
# run `make opt=1`
ifdef opt
CXXFLAGS += -Ofast
endif

all: discovery
discovery: $(LIST) main.cpp
	$(CXX) $(CXXFLAGS) -o discovery $(SOURCEDIR)/main.cpp $(LIST) $(LIBARIES)

$(BIN)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f discovery *.o bin/*
