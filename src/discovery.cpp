#include <iostream>
#include <iomanip>

#include "discovery.h"

void PrintKeys(u16);
int debug = 0;

int main(int argc, char **argv)
{
    std::cout << "Gameboy emulator!" << "\n";
    Discovery emulator;

    // load bios
    if (!emulator.mem->LoadBios())
    {
        std::cerr << "Error loading BIOS\n";
        return 1;
    }

    std::string deb = "";

    // debug mode
    if (argc > 2)
        deb += argv[2];

    if (deb == "-d")
        debug = 1;
    
    emulator.LoadRom(argv[1]);
    return 0;
}

Discovery::Discovery()
{
    cpu = new Arm7Tdmi();
    mem = new Memory();
    gpu = new GPU();
    
    cpu->mem = mem;
    gpu->mem = mem;

    // initialize gamepad buttons to 1 (released)
    Gamepad.a = 1;
    Gamepad.b = 1;
    Gamepad.sel = 1;
    Gamepad.start = 1;
    Gamepad.right = 1;
    Gamepad.left = 1;
    Gamepad.up = 1;
    Gamepad.down = 1;
    Gamepad.r = 1;
    Gamepad.l = 1;
    Gamepad.keys = 0x3FF;

    system_cycles = 0;

    stat = new LcdStat();
    mem->stat = stat;
    gpu->stat = stat;

    // initialize timers
    Timer *t0 = new Timer();
    Timer *t1 = new Timer();
    Timer *t2 = new Timer();
    Timer *t3 = new Timer();

    timers[0] = t0;
    timers[1] = t1;
    timers[2] = t2;
    timers[3] = t3;
    
    // link system's timers to memory's
    mem->timers[0] = timers[0];
    mem->timers[1] = timers[1];
    mem->timers[2] = timers[2];
    mem->timers[3] = timers[3];
}

void Discovery::GameLoop()
{
    SDL_Event e;
    u32 old_cycles = 0;
    int num = 0;
    bool valid;

    while (true)
    {
        cpu->Fetch();
        cpu->Decode(cpu->pipeline[0]);
        cpu->Execute(cpu->pipeline[0]);

        cpu->HandleInerrupt();

        // update pipeline
        cpu->pipeline[0] = cpu->pipeline[1];
        cpu->pipeline[1] = cpu->pipeline[2];

        // run gpu and timers for as many clock cycles as cpu used
        system_cycles = cpu->cycles;
        for (int i = system_cycles - old_cycles; i > 0; --i)
        {
            ++old_cycles;

            gpu->Tick();

            // clock timers
            for (int j = 0; j < 4; ++j)
            {
                // ignore if timer is disabled
                if (!timers[j]->enable)
                    continue;
                
                // ignore if cascade bit is set (timer will be incremented by previous timer)
                if (timers[j]->cascade)
                    continue;
                
                // increment counter by 1
                if (old_cycles % timers[j]->actual_freq == 0)
                {
                    timers[j]->data += 1; // increment timer

                    // timer overflowed
                    if (timers[j]->data == 0x0000)
                    {
                        // reset timer
                        timers[j]->data = timers[j]->start_data;

                        //std::cout << "Timer " << j << " overflow\n";

                        // overflow irq
                        if (timers[j]->irq)
                            std::cout << "Timer " << j << " overflow IRQ request\n";

                        // cascade
                        // timer 4 can't cascade any other timer
                        if (j == 4)
                            continue;
                        
                        if (timers[j + 1]->enable && timers[j + 1]->cascade)
                        {
                            timers[j + 1]->data += 1;

                            // cascade caused overflow (deal with this later)
                            if (timers[j + 1]->data == 0x0000)
                                std::cout << "Timer " << (j + 1) << " overflow\n";
                        }
                    }
                }
            }
        }

        // poll for key presses at start of vblank
        if (stat->scanline == VDRAW && SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                break;
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
                PollKeys(e);
        }

        valid = false;
        // check for valid cpsr
        // switch (cpu->get_mode())
        // {
        //     case Mode::USR:
        //     case Mode::FIQ:
        //     case Mode::IRQ:
        //     case Mode::SVC:
        //     case Mode::ABT:
        //     case Mode::SYS:
        //     case Mode::UND:
        //         valid = true;
        // }

        // if (!valid)  {
        //     std::cerr << "Invalid state in cpsr: " << (int) cpu->registers.cpsr.raw << "\n";
        //     exit(270);
        // }

    }
    
    ShutDown();
}

void Discovery::LoadRom(char *name)
{
    if (!mem->LoadRom(name))
    {
        std::cerr << "Error loading ROM: " << name << "\n";
        exit(1);
    }

    // if (debug)
    //     game_loop_debug();
    
    // else
    GameLoop();
}

// write current key state to KEYINPUT register
void Discovery::PollKeys(const SDL_Event &e)
{
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
    if (e.type == SDL_KEYDOWN)
    {
        switch(e.key.keysym.sym)
        {
            case SDLK_x:         Gamepad.a     = 0; break;
            case SDLK_z:         Gamepad.b     = 0; break;
            case SDLK_BACKSPACE: Gamepad.sel   = 0; break;
            case SDLK_RETURN:    Gamepad.start = 0; break;
            case SDLK_RIGHT:     Gamepad.right = 0; break;
            case SDLK_LEFT:      Gamepad.left  = 0; break;
            case SDLK_UP:        Gamepad.up    = 0; break;
            case SDLK_DOWN:      Gamepad.down  = 0; break;
            case SDLK_s:         Gamepad.r     = 0; break;
            case SDLK_a:         Gamepad.l     = 0; break;
            default: break;
        }
    }

    // poll button releases
    if (e.type == SDL_KEYUP)
    {
        switch(e.key.keysym.sym)
        {
            case SDLK_x:         Gamepad.a     = 1; break;
            case SDLK_z:         Gamepad.b     = 1; break;
            case SDLK_BACKSPACE: Gamepad.sel   = 1; break;
            case SDLK_RETURN:    Gamepad.start = 1; break;
            case SDLK_RIGHT:     Gamepad.right = 1; break;
            case SDLK_LEFT:      Gamepad.left  = 1; break;
            case SDLK_UP:        Gamepad.up    = 1; break;
            case SDLK_DOWN:      Gamepad.down  = 1; break;
            case SDLK_s:         Gamepad.r     = 1; break;
            case SDLK_a:         Gamepad.l     = 1; break;
            default: break;
        }
    }

    u16 gamepad_result = 0;
    gamepad_result |= Gamepad.l     << 9;
    gamepad_result |= Gamepad.r     << 8;
    gamepad_result |= Gamepad.down  << 7;
    gamepad_result |= Gamepad.up    << 6;
    gamepad_result |= Gamepad.left  << 5;
    gamepad_result |= Gamepad.right << 4;
    gamepad_result |= Gamepad.start << 3;
    gamepad_result |= Gamepad.sel   << 2;
    gamepad_result |= Gamepad.b     << 1;
    gamepad_result |= Gamepad.a     << 0;

    Gamepad.keys = gamepad_result;
    
    // check for key interrupt
    u16 keycnt = mem->Read16Unsafe(REG_KEYCNT);
    if (keycnt >> 14 & 0x1) // key interrupts enabled
    {
        u16 keys = keycnt & 0x3FF; // keys to check

        bool raise_interrupt = false;
        if (keycnt >> 15) // use AND (raise if all keys are down)
        {
            // all keys to check are down (high)
            if (keys == ~gamepad_result)
                raise_interrupt = true;
        }

        else // use OR (raise if any keys are down)
        {
            for (int i = 0; i < 10; ++i) // 10 keys
            {
                if ((keys >> i & 1) && (gamepad_result >> i & 1) == 0) 
                {
                    raise_interrupt = true;
                    break;
                }
            }
        }

        // raise keypad interrupt
        if (raise_interrupt)
        {
            std::cout << "Raising gamepad interrupt\n";
            u16 reg_if = mem->Read16Unsafe(REG_IF) | IRQ_KEYPAD;
            mem->Write16Unsafe(REG_IF, reg_if);
        }
    }

    // store gamepad result back into the KEYINPUT address
    mem->Write32Unsafe(REG_KEYINPUT, gamepad_result);
}

void PrintKeys(u16 keys)
{
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

void Discovery::ShutDown()
{
    // free resources and shutdown
    delete mem;

    for (int i = 0; i < 4; ++i)
        delete timers[i];
        
    cpu->~Arm7Tdmi();
    gpu->~GPU();
}

// void discovery::game_loop_debug()
// {
//     SDL_Event e;
//     u32 old_cycles = 0;
//     int num = 0;
//     std::string input;
//     u32 breakpoint;
//     while (true)
//     {
//         std::cout << "> ";
//         std::getline(std::cin, input);

//         switch (input.at(0))
//         {
//             case 'n':
//                 cpu->Fetch();
//                 cpu->Decode(cpu->pipeline[0]);
//                 cpu->Execute(cpu->pipeline[0]);
//                 print_debug_info();
//                 break;

//             case 'b':
//                 breakpoint = std::stoi(input.substr(2));
//                 continue;
//                 break;
            
//             case 'c':
//                 while (cpu->Registers.r15 != breakpoint)
//                 {
//                     cpu->Fetch();
//                     cpu->Decode(cpu->pipeline[0]);
//                     cpu->Execute(cpu->pipeline[0]);

//                     cpu->pipeline[0] = cpu->pipeline[1];
//                     cpu->pipeline[1] = cpu->pipeline[2];

//                     // run gpu for as many clock cycles as cpu used
//                     system_cycles = cpu->cycles;
//                     for (int i = system_cycles - old_cycles; i > 0; --i)
//                         gpu->Tick();
//                     old_cycles = system_cycles;
                
//                     // poll for key presses at start of vblank
//                     if (stat->scanline == 160 && SDL_PollEvent(&e))
//                     {
//                         if (e.type == SDL_QUIT)
//                             break;
//                     }
//                 }
//                 print_debug_info();
//                 continue;
//         }
        

//         // update pipeline
//         cpu->pipeline[0] = cpu->pipeline[1];
//         cpu->pipeline[1] = cpu->pipeline[2];

//         // run gpu for as many clock cycles as cpu used
//         system_cycles = cpu->cycles;
//         for (int i = system_cycles - old_cycles; i > 0; --i)
//             gpu->Tick();
//         old_cycles = system_cycles;
    
//         // poll for key presses at start of vblank
//         if (gpu->stat->scanline == 160 && SDL_PollEvent(&e))
//         {
//             if (e.type == SDL_QUIT)
//                 break;
//         }

//         cpu->HandleInerrupt();
//     }
//     shutdown();
// }

// void discovery::print_debug_info()
// {
//     std::cout << "Executed: " << std::hex << cpu->pipeline[0] << "\n";

//     //print registers
//     std::cout<< std::hex <<"R0 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(0) << 
// 				" -- R4  : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(4) << 
// 				" -- R8  : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(8) << 
// 				" -- R12 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(12) << "\n";

// 			std::cout<< std::hex <<"R1 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(1) << 
// 				" -- R5  : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(5) << 
// 				" -- R9  : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(9) << 
// 				" -- R13 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(13) << "\n";

// 			std::cout<< std::hex <<"R2 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(2) << 
// 				" -- R6  : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(6) << 
// 				" -- R10 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(10) << 
// 				" -- R14 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(14) << "\n";

// 			std::cout<< std::hex <<"R3 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(3) << 
// 				" -- R7  : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(7) << 
// 				" -- R11 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(11) << 
// 				" -- R15 : 0x" << std::setw(8) << std::setfill('0') << cpu->GetRegister(15) << "\n";

	
// 			std::cout<< std::hex <<"CPSR : 0x" << std::setw(8) << std::setfill('0') << cpu->Registers.cpsr.raw << "\t";
//             // if (cpu->get_condition_code_flag(ConditionFlag::N))
//             //     std::cout << "N";
//             // if (cpu->get_condition_code_flag(ConditionFlag::Z))
//             //     std::cout << "Z";
//             // if (cpu->get_condition_code_flag(ConditionFlag::C))
//             //     std::cout << "C";
//             // if (cpu->get_condition_code_flag(ConditionFlag::V))
//             //     std::cout << "V";
//             std::cout << "\n";
//             std::cout << "Cycles: " << std::dec << cpu->cycles << "\n";
// }