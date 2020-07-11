all: discovery test

discovery: arm_7tdmi.o cpu.o util.o memory.o discovery.cpp
	g++ -g -o discovery arm_7tdmi.o cpu.o util.o memory.o discovery.cpp

test: arm_7tdmi.o cpu.o util.o cpu/tests/tests.cpp cpu/tests/instruction_tests.cpp cpu/tests/data_processing_tests.cpp
	g++ -o test arm_7tdmi.o cpu.o util.o cpu/tests/tests.cpp cpu/tests/instruction_tests.cpp cpu/tests/data_processing_tests.cpp

arm_7tdmi.o: cpu/arm_7tdmi.h cpu/arm_7tdmi.cpp
	g++ -g -c cpu/arm_7tdmi.cpp cpu/arm_alu.inl

cpu.o: cpu/cpu.h cpu/cpu.cpp
	g++ -c cpu/cpu.cpp

util.o: cpu/util.h cpu/util.cpp
	g++ -c cpu/util.cpp

memory.o: memory/memory.h memory/memory.cpp
	g++ -g -c memory/memory.cpp

clean:
	rm -f discovery test *.o
