LIBARIES = -lstdc++fs -l SDL2
# CFLAGS = 

all: discovery

discovery: arm_7tdmi.o util.o memory.o gpu.o discovery.cpp
	g++ -g -o discovery arm_7tdmi.o util.o memory.o gpu.o discovery.cpp $(LIBARIES)

test: arm_7tdmi.o util.o cpu/tests/tests.cpp cpu/tests/instruction_tests.cpp cpu/tests/data_processing_tests.cpp
	g++ -o test arm_7tdmi.o util.o memory.o cpu/tests/tests.cpp cpu/tests/instruction_tests.cpp cpu/tests/data_processing_tests.cpp $(LIBARIES)

arm_7tdmi.o: cpu/arm_7tdmi.h cpu/arm_7tdmi.cpp
	g++ -g -c cpu/arm_7tdmi.cpp cpu/arm_alu.inl

util.o: cpu/util.h cpu/util.cpp
	g++ -g -c cpu/util.cpp

memory.o: memory/memory.h memory/memory.cpp
	g++ -g -c memory/memory.cpp

gpu.o: gpu/gpu.h gpu/gpu.cpp
	g++ -g -c gpu/gpu.cpp

clean:
	rm -f discovery test *.o
