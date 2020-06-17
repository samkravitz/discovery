tests: arm_7tdmi.o cpu.o instruction.o cpu/tests/tests.cpp
	g++ -o tests arm_7tdmi.o cpu.o instruction.o cpu/tests/tests.cpp

arm_7tdmi.o: cpu/arm_7tdmi.h cpu/arm_7tdmi.cpp
	g++ -c cpu/arm_7tdmi.cpp

cpu.o: cpu/cpu.h cpu/cpu.cpp
	g++ -c cpu/cpu.cpp

instruction.o: cpu/instruction.h cpu/instruction.cpp
	g++ -c cpu/instruction.cpp


clean:
	rm -f tests *.o
