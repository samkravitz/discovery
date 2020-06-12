tests: cpu/tests/tests.cpp cpu/arm.h cpu/arm.cpp cpu/common.h cpu/cpu.h cpu/cpu.cpp
	g++ -g -o tests cpu/tests/tests.cpp cpu/arm.h cpu/arm.cpp cpu/common.h cpu/cpu.h cpu/cpu.cpp