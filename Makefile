CXX = g++
LIBARIES = -lstdc++fs -lSDL2 -DFMT_HEADER_ONLY
CXXFLAGS = -g -std=c++2a -I $(INCLUDEDIR)
BIN = bin/
SOURCEDIR = src/
INCLUDEDIR = include/
OBJECTS = Arm7Tdmi.o util.o Memory.o PPU.o Gamepad.o Timer.o IRQ.o APU.o HandlerArm.o HandlerThumb.o swi.o Flash.o
LIST=$(addprefix $(BIN), $(OBJECTS))
VPATH = $(SOURCEDIR)
TESTS = $(SOURCEDIR)tests/tests.cpp $(SOURCEDIR)tests/instruction_tests.cpp $(SOURCEDIR)tests/data_processing_tests.cpp

# Use compiler optimizations
OPT = 0

ifeq (1, $(OPT))
CXXFLAGS += -Ofast
endif

all: discovery

discovery: $(LIST) Discovery.cpp
	$(CXX) $(CXXFLAGS) -o discovery $(SOURCEDIR)Discovery.cpp $(LIST) $(LIBARIES)

test: $(OBJECTS) $(TESTS)
	 $(CXX) -o test $(OBJECTS) $(TESTS) $(LIBARIES)

$(BIN)%.o : %.cpp
	@printf "Compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@printf "%b\n%b" "$(OK_COLOR) $(OK_STRING) $(NO_COLOR)"

.PHONY: clean
clean:
	rm -f discovery *.o bin/*

COM_COLOR   = \033[0;34m
OBJ_COLOR   = \033[0;36m
OK_COLOR    = \033[0;32m
ERROR_COLOR = \033[0;31m
WARN_COLOR  = \033[0;33m
NO_COLOR    = \033[m

OK_STRING    = "[OK]"
ERROR_STRING = "[ERROR]"
WARN_STRING  = "[WARNING]"
COM_STRING   = "Compiling"
