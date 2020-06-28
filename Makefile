tests: arm_7tdmi.o cpu.o util.o cpu/tests/tests.cpp
	g++ -o tests arm_7tdmi.o cpu.o util.o cpu/tests/tests.cpp

arm_7tdmi.o: cpu/arm_7tdmi.h cpu/arm_7tdmi.cpp
	g++ -c cpu/arm_7tdmi.cpp cpu/arm_alu.inl

cpu.o: cpu/cpu.h cpu/cpu.cpp
	g++ -c cpu/cpu.cpp

util.o: cpu/util.h cpu/util.cpp
	g++ -c cpu/util.cpp

clean:
	rm -f tests *.o
