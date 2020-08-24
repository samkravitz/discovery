#include <iostream>
#include <ctime>
#include "discovery.h"

void print_keys(u32);

discovery::discovery() {
    mem = new Memory();
    cpu.mem = mem;
    gpu.mem = mem;

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
    while (true) {
        cpu.fetch();
        cpu.decode(cpu.pipeline[0]);
        cpu.execute(cpu.pipeline[0]);
        // std::cout << "Executed: " << std::hex << cpu.pipeline[0] << "\n";

        // update pipeline
        cpu.pipeline[0] = cpu.pipeline[1];
        cpu.pipeline[1] = cpu.pipeline[2];

        // TODO - need a much better timing system
        if (clock() % 60000 < 3) { // 60000 milliseconds per draw
            gpu.draw();
            poll_event();
        }
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
    return 0;
}

// handle events such as key presses or X-ing out of discovery
void discovery::poll_event() {
    SDL_Event e;
    SDL_PollEvent(&e);

    // check if close button has been clicked
    if (e.type == SDL_QUIT) exit(0);

    // poll button presses
    if (e.type == SDL_KEYDOWN) {
        switch(e.key.keysym.sym) {
            case SDLK_x: gamepad.a = 0; break;
            case SDLK_z: gamepad.b = 0; break;
            case SDLK_BACKSPACE: gamepad.sel = 0; break;
            case SDLK_RETURN: gamepad.start = 0; break;
            case SDLK_RIGHT: gamepad.right = 0; break;
            case SDLK_LEFT: gamepad.left = 0; break;
            case SDLK_UP: gamepad.up = 0; break;
            case SDLK_DOWN: gamepad.down = 0; break;
            case SDLK_s: gamepad.r = 0; break;
            case SDLK_a: gamepad.l = 0; break;
            default: break;
        }
    }

    // poll button releases
    if (e.type == SDL_KEYUP) {
        switch(e.key.keysym.sym) {
            case SDLK_x: gamepad.a = 1; break;
            case SDLK_z: gamepad.b = 1; break;
            case SDLK_BACKSPACE: gamepad.sel = 1; break;
            case SDLK_RETURN: gamepad.start = 1; break;
            case SDLK_RIGHT: gamepad.right = 1; break;
            case SDLK_LEFT: gamepad.left = 1; break;
            case SDLK_UP: gamepad.up = 1; break;
            case SDLK_DOWN: gamepad.down = 1; break;
            case SDLK_s: gamepad.r = 1; break;
            case SDLK_a: gamepad.l = 1; break;
            default: break;
        }
    }

    u32 gamepad_result = 0;
    gamepad_result |= gamepad.l << 9;
    gamepad_result |= gamepad.r << 8;
    gamepad_result |= gamepad.down << 7;
    gamepad_result |= gamepad.up << 6;
    gamepad_result |= gamepad.left << 5;
    gamepad_result |= gamepad.right << 4;
    gamepad_result |= gamepad.start << 3;
    gamepad_result |= gamepad.sel << 2;
    gamepad_result |= gamepad.b << 1;
    gamepad_result |= gamepad.a;

    // print_keys(gamepad_result);

    // store gamepad result back into the KEYINPUT address
    mem->write_u32(REG_KEYINPUT, gamepad_result);
}

void print_keys(u32 keys) {
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
