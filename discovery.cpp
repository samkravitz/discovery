#include <iostream>
#include <fstream>
#include "discovery.h"
#include "cpu/arm_7tdmi.h"
#include "memory/memory.h"

discovery::discovery() {
    cpu.mem = new Memory();
    gpu.mem = cpu.mem;
}

void discovery::game_loop() {
    while (true) {
        cpu.fetch();
        cpu.decode(cpu.pipeline[0]);
        cpu.execute(cpu.pipeline[0]);
        std::cout << "Executed: " << std::hex << cpu.pipeline[0] << "\n";

        // update pipeline
        cpu.pipeline[0] = cpu.pipeline[1];
        cpu.pipeline[1] = cpu.pipeline[2];

        // TODO - need a much better timing system
        //if (cpu.registers.r15 == 134218482) {
        // gpu.draw();
        //dd}

    }
}

void discovery::run_asm(char *name) {
    cpu.mem->load_rom(name);
    game_loop();
}

int main(int argc, char **argv) {
    std::cout << "Gameboy emulator!" << "\n";
    discovery emulator;
    emulator.run_asm(argv[1]);
    // while (true) {
    //     emulator.gpu.draw_pixel(120, 80);
    // }
    return 0;
}
