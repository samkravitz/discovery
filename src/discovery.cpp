#include <iostream>
#include <ctime>
#include "discovery.h"
#include "common/common.h"

void print_keys(u16);

discovery::discovery() {
    mem = new Memory();
    cpu.mem = mem;
    cpu.gpu.mem = mem;

    // initialize gamepad buttons to 1 (released)
    gamepad.a = 1;
    gamepad.b = 1;
    gamepad.sel = 1;
    gamepad.start = 1;
    gamepad.right = 1;
    gamepad.left = 1;
    gamepad.up = 1;
    gamepad.down = 1;
    gamepad.r = 1;
    gamepad.l = 1;
}

void discovery::game_loop() {
    SDL_Event e;

    while (true) {
        cpu.fetch();
        cpu.decode(cpu.pipeline[0]);
        cpu.execute(cpu.pipeline[0]);
        //std::cout << "Executed: " << std::hex << cpu.pipeline[0] << "\n";

        // update pipeline
        cpu.pipeline[0] = cpu.pipeline[1];
        cpu.pipeline[1] = cpu.pipeline[2];

        // TODO - need a much better timing system
        // poll for key presses during vblank
        if (cpu.gpu.current_scanline == 160 && SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) break;
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) poll_keys(e);
        }

        cpu.handle_interrupt();
    }

    shutdown();
}

void discovery::run_asm(char *name) {
    cpu.mem->load_rom(name);
    game_loop();
}

int main(int argc, char **argv) {
    std::cout << "Gameboy emulator!" << "\n";
    discovery emulator;
    emulator.mem->load_bios();
    emulator.run_asm(argv[1]);
    return 0;
}

// write current key state to KEYINPUT register
void discovery::poll_keys(SDL_Event e) {
    /*
     * Order of keys in KEYINPUT is as follows:
     * a: 0
     * b: 1
     * select: 2
     * start: 3
     * right: 4
     * left: 5
     * up: 6
     * down: 7
     * r: 8
     * l: 9
     */
    // poll button presses
    if (e.type == SDL_KEYDOWN) {
        switch(e.key.keysym.sym) {
            case SDLK_x:         gamepad.a     = 0; break;
            case SDLK_z:         gamepad.b     = 0; break;
            case SDLK_BACKSPACE: gamepad.sel   = 0; break;
            case SDLK_RETURN:    gamepad.start = 0; break;
            case SDLK_RIGHT:     gamepad.right = 0; break;
            case SDLK_LEFT:      gamepad.left  = 0; break;
            case SDLK_UP:        gamepad.up    = 0; break;
            case SDLK_DOWN:      gamepad.down  = 0; break;
            case SDLK_s:         gamepad.r     = 0; break;
            case SDLK_a:         gamepad.l     = 0; break;
            default: break;
        }
    }

    // poll button releases
    if (e.type == SDL_KEYUP) {
        switch(e.key.keysym.sym) {
            case SDLK_x:         gamepad.a     = 1; break;
            case SDLK_z:         gamepad.b     = 1; break;
            case SDLK_BACKSPACE: gamepad.sel   = 1; break;
            case SDLK_RETURN:    gamepad.start = 1; break;
            case SDLK_RIGHT:     gamepad.right = 1; break;
            case SDLK_LEFT:      gamepad.left  = 1; break;
            case SDLK_UP:        gamepad.up    = 1; break;
            case SDLK_DOWN:      gamepad.down  = 1; break;
            case SDLK_s:         gamepad.r     = 1; break;
            case SDLK_a:         gamepad.l     = 1; break;
            default: break;
        }
    }

    u16 gamepad_result = 0;
    gamepad_result |= gamepad.l     << 9;
    gamepad_result |= gamepad.r     << 8;
    gamepad_result |= gamepad.down  << 7;
    gamepad_result |= gamepad.up    << 6;
    gamepad_result |= gamepad.left  << 5;
    gamepad_result |= gamepad.right << 4;
    gamepad_result |= gamepad.start << 3;
    gamepad_result |= gamepad.sel   << 2;
    gamepad_result |= gamepad.b     << 1;
    gamepad_result |= gamepad.a     << 0;

    print_keys(gamepad_result);

    // // store gamepad result back into the KEYINPUT address
    // // mem->write_u32(REG_KEYINPUT, gamepad_result);
    // // request keyboard interrupt
    // u32 reg_if = mem->read_u32(REG_IF);
    // reg_if |= 1 << 13;
    // mem->write_u32(REG_IF, reg_if);
}

void print_keys(u16 keys) {
    std::cout << "\n\n";
    if (((keys >> 9) & 1) == 0) std::cout << "L is pressed\n";
    if (((keys >> 8) & 1) == 0) std::cout << "R is pressed\n";
    if (((keys >> 7) & 1) == 0) std::cout << "Down is pressed\n";
    if (((keys >> 6) & 1) == 0) std::cout << "Up is pressed\n";
    if (((keys >> 5) & 1) == 0) std::cout << "Left is pressed\n";
    if (((keys >> 4) & 1) == 0) std::cout << "Right is pressed\n";
    if (((keys >> 3) & 1) == 0) std::cout << "Start is pressed\n";
    if (((keys >> 2) & 1) == 0) std::cout << "Select is pressed\n";
    if (((keys >> 1) & 1) == 0) std::cout << "b is pressed\n";
    if (((keys >> 0) & 1) == 0) std::cout << "a is pressed\n";
}

void discovery::shutdown() {
    // free resources and shutdown
    delete mem;
    cpu.~arm_7tdmi();
}