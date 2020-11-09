/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: memory.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of memory related functions
 */
#include <fstream>
#include <iostream>
#include <experimental/filesystem>

#include "memory.h"

namespace fs = std::experimental::filesystem;

Memory::Memory()
{
    game_rom  = NULL;
    stat      = NULL;
    timers[0] = NULL;
    timers[1] = NULL;
    timers[2] = NULL;
    timers[3] = NULL;
    reset();
}

Memory::~Memory() { }

void Memory::reset()
{
    // default cycle accesses for wait statae
    n_cycles = 4;
    s_cycles = 2;

    rom_size = 0;

    // zero memory
    for (int i = 0; i < MEM_SIZE; ++i)
        memory[i] = 0;
    
    // zero dma
    for (int i = 0; i < 4; ++i)
    {
        dma[i].num_transfers    = 0;
        dma[i].dest_adjust      = 0; 
        dma[i].src_adjust       = 0;  
        dma[i].repeat           = 0;
        dma[i].chunk_size       = 0;
        dma[i].mode             = 0;
        dma[i].irq              = 0;
        dma[i].enable           = 0;

        dma[i].src_address      = 0;
        dma[i].dest_address     = 0;
    }
    
    // write all 1s to keypad (all keys cleared)
    write_u32_unprotected(REG_KEYINPUT, 0b1111111111);
}

u32 Memory::read_u32(u32 address)
{
    return (read_u8(address + 3) << 24)
    | (read_u8(address + 2) << 16)
    | (read_u8(address + 1) << 8)
    | read_u8(address);
}

u16 Memory::read_u16(u32 address)
{
    return (read_u8(address + 1) << 8) | read_u8(address);
}

u8 Memory::read_u8(u32 address)
{
    // game rom
    if (address >= MEM_SIZE)
    {
        // while ((address - MEM_SIZE) >= rom_size)
        // {
        //     address -= 0x2000000;
        // }

        // SRAM
        if (address >= 0xE000000)
        {
            std::cout << "SRAM\n";
            address -= 0x6000000;
        }

        // rom image 2
        else if (address >= 0xC000000)
        {
            address -= 0x4000000;
        }

        // rom image 1
        else if (address >= 0xA000000)
        {
            address -= 0x2000000;
        }

        return game_rom[address - MEM_SIZE];
    }

    // memory mirrors
    // EWRAM
    if (address > MEM_EWRAM_END && address < MEM_IWRAM_START)
    {   
        address &= ~MEM_EWRAM_SIZE;
    }

    // IWRAM
    else if (address > MEM_IWRAM_END && address < MEM_IO_REG_START)
    {   
        address &= ~MEM_IWRAM_SIZE;
    }

    // Palette RAM
    else if (address > MEM_PALETTE_RAM_END && address < MEM_VRAM_START)
    {
           address &= ~MEM_PALETTE_RAM_SIZE;
    }

    // VRAM
    else if (address > MEM_VRAM_END && address < MEM_OAM_START)
    {
        //x06010000 - 0x06017FFF is mirrored from 0x06018000 - 0x0601FFFF.
        if (address <= 0x601FFFF)
        {
            address -= 0x8000;
        }

        // otherwise mirrors every 0x20000
        else
        {
            while (address > MEM_VRAM_END)
                address -= MEM_VRAM_SIZE;   
        }
    }

    // OAM
    else if (address > MEM_OAM_END && address < MEM_SIZE)
    {   
        while (address > MEM_OAM_END)
            address -= MEM_OAM_SIZE;
    }

    u8 result = 0;
    switch (address)
    {
        // IO reg
        case REG_DISPSTAT:
            result |= stat->dispstat.in_vBlank ? 0b1      : 0b0;       // bit 0 set in vblank, clear in vdraw
            result |= stat->dispstat.in_hBlank ? 0b10     : 0b00;      // bit 1 set in hblank, clear in hdraw
            result |= stat->dispstat.vcs       ? 0b100    : 0b000;     // bit 2
            result |= stat->dispstat.vbi       ? 0b1000   : 0b0000;    // bit 3
            result |= stat->dispstat.hbi       ? 0b10000  : 0b00000;   // bit 4
            result |= stat->dispstat.vci       ? 0b100000 : 0b000000;  // bit 5
            return result;
        
        case REG_DISPSTAT + 1:
            return stat->dispstat.vct;

        case REG_VCOUNT:
            return stat->scanline;
        
        // REG_TM0D
        case REG_TM0D:
            return timers[0]->data & 0xFF;
        case REG_TM0D + 1:
            return (timers[0]->data >> 8) & 0xFF;
        
        // REG_TM1D
        case REG_TM1D:
            return timers[1]->data & 0xFF;
        case REG_TM1D + 1:
            return (timers[1]->data >> 8) & 0xFF;

        // REG_TM2D
        case REG_TM2D:
            return timers[2]->data & 0xFF;
        case REG_TM2D + 1:
            return (timers[2]->data >> 8) & 0xFF;

        // REG_TM3D
        case REG_TM3D:
            return timers[3]->data & 0xFF;
        case REG_TM3D + 1:
            return (timers[3]->data >> 8) & 0xFF;

        default:
            return memory[address];
    }
}

void Memory::write_u32(u32 address, u32 value)
{
    write_u8(address    , (value >>  0) & 0xFF);
    write_u8(address + 1, (value >>  8) & 0xFF);
    write_u8(address + 2, (value >> 16) & 0xFF);
    write_u8(address + 3, (value >> 24) & 0xFF);
}

void Memory::write_u16(u32 address, u16 value)
{
    write_u8(address    , (value >> 0) & 0xFF);
    write_u8(address + 1, (value >> 8) & 0xFF);
}

void Memory::write_u8(u32 address, u8 value)
{
    // game rom
    if (address >= MEM_SIZE)
    {
        // SRAM
        if (address >= 0xE000000)
        {
            address -= 0x6000000;
        }
        
        // rom image 2
        if (address >= 0xC000000)
        {
            address -= 0x4000000;
        }

        // rom image 1
        else if (address >= 0xA000000)
        {
            address -= 0x2000000;
        }

        std::cerr << "Warning: writing to game rom\n";

        game_rom[address - MEM_SIZE] = value;
        std::cerr << "Done\n";
        return;
    }

    // memory mirrors
    // EWRAM
    if (address > MEM_EWRAM_END && address < MEM_IWRAM_START)
    {
        while (address > MEM_EWRAM_END)
            address -= MEM_EWRAM_SIZE;
    }

    // IWRAM
    else if (address > MEM_IWRAM_END && address < MEM_IO_REG_START)
    {   
        while (address > MEM_IWRAM_END)
            address -= MEM_IWRAM_SIZE;
    }

    // Palette RAM
    else if (address > MEM_PALETTE_RAM_END && address < MEM_VRAM_START)
    {   
        while (address > MEM_PALETTE_RAM_END)
            address -= MEM_PALETTE_RAM_SIZE;
    }

    // VRAM
    else if (address > MEM_VRAM_END && address < MEM_OAM_START)
    {
        //x06010000 - 0x06017FFF is mirrored from 0x06018000 - 0x0601FFFF.
        if (address <= 0x601FFFF)
        {
            address -= 0x8000;
        }

        // otherwise mirrors every 0x20000
        else
        {
            while (address > MEM_VRAM_END)
                address -= MEM_VRAM_SIZE;   
        }
    }

    // OAM
    else if (address > MEM_OAM_END && address < MEM_GAMEPAK_ROM_START)
    {   
        while (address > MEM_OAM_END)
            address -= MEM_OAM_SIZE;
    }

    switch (address)
    {
        // REG_DISPCNT
        case REG_DISPCNT:
            stat->dispcnt.mode                  = value >> 0 & 0x7; // bits 0-2     
            stat->dispcnt.gb                    = value >> 3 & 0x1; // bit 3
            stat->dispcnt.ps                    = value >> 4 & 0x1; // bit 4
            stat->dispcnt.hb                    = value >> 5 & 0x1; // bit 5
            stat->dispcnt.obj_map_mode          = value >> 6 & 0x1; // bit 6
            stat->dispcnt.fb                    = value >> 7 & 0x1; // bit 7    
        break;

        case REG_DISPCNT + 1:
            stat->bg_cnt[0].enabled             = value >> 0 & 0x1; // bit 8
            stat->bg_cnt[1].enabled             = value >> 1 & 0x1; // bit 9
            stat->bg_cnt[2].enabled             = value >> 2 & 0x1; // bit A
            stat->bg_cnt[3].enabled             = value >> 3 & 0x1; // bit B
            stat->dispcnt.obj_enabled           = value >> 4 & 0x1; // bit C
            stat->dispcnt.win_enabled           = value >> 5 & 0x7; // bits D-F
        break;

        // REG_DISPSTAT
        case REG_DISPSTAT:
            // skip bits 0-2, unwritable
            stat->dispstat.vbi = value >> 3 & 1;
            stat->dispstat.hbi = value >> 4 & 1;
            stat->dispstat.vci = value >> 5 & 1;
        break;

        case REG_DISPSTAT + 1:
            stat->dispstat.vct = value;
        break;

        // REG_BG0CNT
        case REG_BG0CNT:
            stat->bg_cnt[0].priority      = value >> 0 & 0x3; // bits 0-1
            stat->bg_cnt[0].cbb           = value >> 2 & 0x3; // bits 2-3
            stat->bg_cnt[0].mosaic        = value >> 6 & 0x1; // bit  6
            stat->bg_cnt[0].color_mode    = value >> 7 & 0x1; // bit  7
        break;

        case REG_BG0CNT + 1:
            stat->bg_cnt[0].sbb           = value >> 0 & 0x1F; // bits 8-C
            stat->bg_cnt[0].affine_wrap   = value >> 5 & 0x1;  // bit  D
            stat->bg_cnt[0].size          = value >> 6 & 0x3;  // bits E-F
        break;

        // REG_BG1CNT
        case REG_BG1CNT:
            stat->bg_cnt[1].priority      = value >> 0 & 0x3; // bits 0-1
            stat->bg_cnt[1].cbb           = value >> 2 & 0x3; // bits 2-3
            stat->bg_cnt[1].mosaic        = value >> 6 & 0x1; // bit  6
            stat->bg_cnt[1].color_mode    = value >> 7 & 0x1; // bit  7
        break;

        case REG_BG1CNT + 1:
            stat->bg_cnt[1].sbb           = value >> 0 & 0x1F; // bits 8-C
            stat->bg_cnt[1].affine_wrap   = value >> 5 & 0x1;  // bit  D
            stat->bg_cnt[1].size          = value >> 6 & 0x3;  // bits E-F
        break;

        // REG_BG2CNT
        case REG_BG2CNT:
            stat->bg_cnt[2].priority      = value >> 0 & 0x3; // bits 0-1
            stat->bg_cnt[2].cbb           = value >> 2 & 0x3; // bits 2-3
            stat->bg_cnt[2].mosaic        = value >> 6 & 0x1; // bit  6
            stat->bg_cnt[2].color_mode    = value >> 7 & 0x1; // bit  7
        break;

        case REG_BG2CNT + 1:
            stat->bg_cnt[2].sbb           = value >> 0 & 0x1F; // bits 8-C
            stat->bg_cnt[2].affine_wrap   = value >> 5 & 0x1;  // bit  D
            stat->bg_cnt[2].size          = value >> 6 & 0x3;  // bits E-F
        break;

        // REG_BG3CNT
        case REG_BG3CNT:
            stat->bg_cnt[3].priority      = value >> 0 & 0x3; // bits 0-1
            stat->bg_cnt[3].cbb           = value >> 2 & 0x3; // bits 2-3
            stat->bg_cnt[3].mosaic        = value >> 6 & 0x1; // bit  6
            stat->bg_cnt[3].color_mode    = value >> 7 & 0x1; // bit  7
        break;

        case REG_BG3CNT + 1:
            stat->bg_cnt[3].sbb           = value >> 0 & 0x1F; // bits 8-C
            stat->bg_cnt[3].affine_wrap   = value >> 5 & 0x1;  // bit  D
            stat->bg_cnt[3].size          = value >> 6 & 0x3;  // bits E-F
        break;
        
        // write into waitstate ctl
        case WAITCNT:
            switch(value >> 2 & 0b11) // bits 2-3
            {
                case 0: n_cycles = 4; break;
                case 1: n_cycles = 3; break;
                case 2: n_cycles = 2; break;
                case 3: n_cycles = 8; break;
            }

            switch (value >> 4) // bit 4
            {
                case 0: s_cycles = 2; break;
                case 1: s_cycles = 1; break;
            }

        break;
        
        // DMA

        // REG_DMA0CNT
        case REG_DMA0CNT:
            dma[0].num_transfers |= value;
        break;

        case REG_DMA0CNT + 1:
            dma[0].num_transfers |= (value << 8);
        break;

        case REG_DMA0CNT + 2:
            dma[0].dest_adjust    = value >> 5 & 0x3;
            dma[0].src_adjust    |= value >> 6 & 0x1; 
        break;

        case REG_DMA0CNT + 3:
            dma[0].src_adjust    |= ((value & 0x1) << 1);
            dma[0].repeat         = value >> 1 & 0x1;
            dma[0].chunk_size     = value >> 2 & 0x1;
            dma[0].mode           = value >> 4 & 0x3;
            dma[0].irq            = value >> 6 & 0x1;
            dma[0].enable         = value >> 7 & 0x1; 

            if (dma[0].enable && dma[0].mode == 0) // immediate mode
            {
                std::cout << "DMA0 immediate\n";
                _dma(0);

                // disable DMA after immediate transfer
                dma[0].enable == 0;
            }

        break;

        // REG_DMA1CNT
        case REG_DMA1CNT:
            dma[1].num_transfers |= value;
        break;

        case REG_DMA1CNT + 1:
            dma[1].num_transfers |= (value << 8);
        break;

        case REG_DMA1CNT + 2:
            dma[1].dest_adjust    = value >> 5 & 0x3;
            dma[1].src_adjust    |= value >> 6 & 0x1; 
        break;

        case REG_DMA1CNT + 3:
            dma[1].src_adjust    |= ((value & 0x1) << 1);
            dma[1].repeat         = value >> 1 & 0x1;
            dma[1].chunk_size     = value >> 2 & 0x1;
            dma[1].mode           = value >> 4 & 0x3;
            dma[1].irq            = value >> 6 & 0x1;
            dma[1].enable         = value >> 7 & 0x1; 

            if (dma[1].enable && dma[1].mode == 0) // immediate mode
            {
                std::cout << "DMA1 immediate\n";
                _dma(1);

                // disable DMA after immediate transfer
                dma[1].enable == 0;
            }

        break;

        // REG_DMA2CNT
        case REG_DMA2CNT:
            dma[2].num_transfers |= value;
        break;

        case REG_DMA2CNT + 1:
            dma[2].num_transfers |= (value << 8);
        break;

        case REG_DMA2CNT + 2:
            dma[2].dest_adjust    = value >> 5 & 0x3;
            dma[2].src_adjust    |= value >> 6 & 0x1; 
        break;

        case REG_DMA2CNT + 3:
            dma[2].src_adjust    |= ((value & 0x1) << 1);
            dma[2].repeat         = value >> 1 & 0x1;
            dma[2].chunk_size     = value >> 2 & 0x1;
            dma[2].mode           = value >> 4 & 0x3;
            dma[2].irq            = value >> 6 & 0x1;
            dma[2].enable         = value >> 7 & 0x1; 

            if (dma[2].enable && dma[2].mode == 0) // immediate mode
            {
                std::cout << "DMA2 immediate\n";
                _dma(2);

                // disable DMA after immediate transfer
                dma[2].enable == 0;
            }

        break;

        // REG_DMA3CNT
        case REG_DMA3CNT:
            dma[3].num_transfers |= value;
        break;

        case REG_DMA3CNT + 1:
            dma[3].num_transfers |= (value << 8);
        break;

        case REG_DMA3CNT + 2:
            dma[3].dest_adjust    = value >> 5 & 0x3;
            dma[3].src_adjust    |= value >> 6 & 0x1; 
        break;

        case REG_DMA3CNT + 3:
            dma[3].src_adjust    |= ((value & 0x1) << 1);
            dma[3].repeat         = value >> 1 & 0x1;
            dma[3].chunk_size     = value >> 2 & 0x1;
            dma[3].mode           = value >> 4 & 0x3;
            dma[3].irq            = value >> 6 & 0x1;
            dma[3].enable         = value >> 7 & 0x1; 

            if (dma[3].enable && dma[3].mode == 0) // immediate mode
            {
                std::cout << "DMA3 immediate\n";
                _dma(3);

                // disable DMA after immediate transfer
                dma[3].enable == 0;
            }

        break;

        // timers

        // REG_TM0D
        case REG_TM0D:
            timers[0]->data       |= value;
            timers[0]->start_data |= value;
        break;

        case REG_TM0D + 1:
            timers[0]->data       |= (value << 8);
            timers[0]->start_data |= (value << 8);
        break;
        
        // REG_TM1D
        case REG_TM1D:
            timers[1]->data       |= value;
            timers[1]->start_data |= value;
        break;

        case REG_TM1D + 1:
            timers[1]->data       |= (value << 8);
            timers[1]->start_data |= (value << 8);
        break;

        // REG_TM2D
        case REG_TM2D:
            timers[2]->data       |= value;
            timers[2]->start_data |= value;
        break;

        case REG_TM2D + 1:
            timers[2]->data       |= (value << 8);
            timers[2]->start_data |= (value << 8);
        break;

        // REG_TM3D
        case REG_TM3D:
            timers[3]->data       |= value;
            timers[3]->start_data |= value;
        break;

        case REG_TM3D + 1:
            timers[3]->data       |= (value << 8);
            timers[3]->start_data |= (value << 8);
        break;
        
        // REG_TM0CNT
        case REG_TM0CNT:
            timers[0]->freq      |= value      & 0x3;
            timers[0]->cascade    = value >> 2 & 0x1;
            timers[0]->irq        = value >> 6 & 0x1;
            timers[0]->enable     = value >> 7 & 0x1;

            // get actual freq
            switch(timers[0]->freq)
            {
                case 0: timers[0]->actual_freq = 1;    break;
                case 1: timers[0]->actual_freq = 64;   break;
                case 2: timers[0]->actual_freq = 256;  break;
                case 3: timers[0]->actual_freq = 1024; break;
                
            }
        break;

        // REG_TM1CNT
        case REG_TM1CNT:
            timers[1]->freq      |= value      & 0x3;
            timers[1]->cascade    = value >> 2 & 0x1;
            timers[1]->irq        = value >> 6 & 0x1;
            timers[1]->enable     = value >> 7 & 0x1;

            // get actual freq
            switch(timers[1]->freq)
            {
                case 0: timers[1]->actual_freq = 1;    break;
                case 1: timers[1]->actual_freq = 64;   break;
                case 2: timers[1]->actual_freq = 256;  break;
                case 3: timers[1]->actual_freq = 1024; break;
                
            }
        break;
        
        // REG_TM2CNT
        case REG_TM2CNT:
            timers[2]->freq      |= value      & 0x3;
            timers[2]->cascade    = value >> 2 & 0x1;
            timers[2]->irq        = value >> 6 & 0x1;
            timers[2]->enable     = value >> 7 & 0x1;

            // get actual freq
            switch(timers[2]->freq)
            {
                case 0: timers[2]->actual_freq = 1;    break;
                case 1: timers[2]->actual_freq = 64;   break;
                case 2: timers[2]->actual_freq = 256;  break;
                case 3: timers[2]->actual_freq = 1024; break;
                
            }
        break;

        // REG_TM3CNT
        case REG_TM3CNT:
            timers[3]->freq      |= value      & 0x3;
            timers[3]->cascade    = value >> 2 & 0x1;
            timers[3]->irq        = value >> 6 & 0x1;
            timers[3]->enable     = value >> 7 & 0x1;

            // get actual freq
            switch(timers[3]->freq)
            {
                case 0: timers[3]->actual_freq = 1;    break;
                case 1: timers[3]->actual_freq = 64;   break;
                case 2: timers[3]->actual_freq = 256;  break;
                case 3: timers[3]->actual_freq = 1024; break;
            }
        break;
                
    }

    // write value at memory location
    memory[address] = value;
}

u32 Memory::read_u32_unprotected(u32 address)
{
    return (read_u8_unprotected(address + 3) << 24)
    | (read_u8_unprotected(address + 2) << 16)
    | (read_u8_unprotected(address + 1) << 8)
    | read_u8_unprotected(address);
}

u16 Memory::read_u16_unprotected(u32 address)
{
    return (read_u8_unprotected(address + 1) << 8) | read_u8_unprotected(address);
}

u8 Memory::read_u8_unprotected(u32 address)
{
    return memory[address];
}

void Memory::write_u32_unprotected(u32 address, u32 value)
{
    write_u8_unprotected(address, value & 0xFF);
    write_u8_unprotected(address + 1, (value >> 8) & 0xFF);
    write_u8_unprotected(address + 2, (value >> 16) & 0xFF);
    write_u8_unprotected(address + 3, (value >> 24) & 0xFF);
}

void Memory::write_u16_unprotected(u32 address, u16 value)
{
    write_u8_unprotected(address, value & 0xFF);
    write_u8_unprotected(address + 1, (value >> 8) & 0xFF);
}

void Memory::write_u8_unprotected(u32 address, u8 value)
{
    memory[address] = value;
}

bool Memory::load_rom(char *name)
{
    std::ifstream rom(name, std::ios::in | std::ios::binary);

    if (!rom)
        return false;

    rom_size = fs::file_size(name);

    if (!rom.good())
    {
        std::cerr << "Bad rom!" << "\n";
        return false;
    }

    game_rom = new u8[rom_size]();
    rom.read((char *) game_rom, rom_size);
    rom.close();
    return true;
}

bool Memory::load_bios()
{
    // bios must be called gba_bios.bin
    std::ifstream bios("gba_bios.bin", std::ios::in | std::ios::binary);
    if (!bios)
        return false;

    if (!bios.good())
    {
        std::cerr << "Bad bios!" << "\n";
        return false;
    }

    bios.read((char *) memory, MEM_BIOS_SIZE);
    bios.close();
    return true;
}

void Memory::_dma(int n)
{
    switch (n)
    {
        case 0: dma0(); break;
        case 1: dma1(); break;
        case 2: dma2(); break;
        case 3: dma3(); break;

        default: // should never happen
            std::cerr << "Error: accessing unknown DMA: " << n << "\n";
    }
}

void Memory::dma0()
{
    std::cout << "DMA 0\n";
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = read_u32_unprotected(REG_DMA3SAD) & 0x7FFFFFF; // 27 bit
    dest_ptr = original_dest = read_u32_unprotected(REG_DMA3DAD) & 0x7FFFFFF; // 27 bit;

    // increment for destination, src
    int dest_inc, src_inc;

    // get increment mode for destination
    switch (dma[0].dest_adjust)
    {
        case 0:
            dest_inc = 1;  // increment after each copy
        break;

        case 1:
            dest_inc = -1; // decrement after each copy
        break;

        case 2:
            dest_inc = 0;  // leave unchanged
        break;

        case 3:
            dest_inc = 1;  // increment after each copy, reset after transfer
        break;
        
        default: // should never happen
            std::cout << "Error: Illegal option for DMA 0 destination adjust " << dma[0].dest_adjust << "\n";
        break;
    }
    
    // get increment mode for destination
    switch (dma[0].src_adjust)
    {
        case 0:
            src_inc = 1;  // increment after each copy
        break;

        case 1:
            src_inc = -1; // decrement after each copy
        break;

        case 2:
            src_inc = 0;  // leave unchanged
        break;
        
        default: // should never happen
            std::cout << "Error: Illegal option for DMA 0 src adjust " << dma[0].src_adjust << "\n";
        break;
    }
    
    // 32 bit copy
    if (dma[0].chunk_size == 1)
    {
        for (int i = 0; i < dma[0].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write_u32(dest_ptr, read_u32(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u32);
            dest_ptr += dest_inc * sizeof(u32);
        }
    }

    // 16 bit copy
    else
    {
        for (int i = 0; i < dma[0].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write_u16(dest_ptr, read_u16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[0].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    write_u32_unprotected(REG_DMA3DAD, dest_ptr);

    // write back src
    write_u32_unprotected(REG_DMA3SAD, src_ptr);
    

    // turn off this transfer if repeat bit is not set
    if (dma[0].repeat == 0)
        dma[0].enable == 0;
    
    // IRQ request
    if (dma[0].irq)
        std::cout << "DMA0 IRQ request\n";
    
    std::cout << "DMA 0 Done\n";
}

void Memory::dma1()
{
    std::cout << "DMA 1\n";
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = read_u32_unprotected(REG_DMA3SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = read_u32_unprotected(REG_DMA3DAD) & 0x7FFFFFF; // 27 bit;

    // increment for destination, src
    int dest_inc, src_inc;

    // get increment mode for destination
    switch (dma[1].dest_adjust)
    {
        case 0:
            dest_inc = 1;  // increment after each copy
        break;

        case 1:
            dest_inc = -1; // decrement after each copy
        break;

        case 2:
            dest_inc = 0;  // leave unchanged
        break;

        case 3:
            dest_inc = 1;  // increment after each copy, reset after transfer
        break;
        
        default: // should never happen
            std::cout << "Error: Illegal option for DMA 1 destination adjust " << dma[1].dest_adjust << "\n";
        break;
    }
    
    // get increment mode for destination
    switch (dma[1].src_adjust)
    {
        case 0:
            src_inc = 1;  // increment after each copy
        break;

        case 1:
            src_inc = -1; // decrement after each copy
        break;

        case 2:
            src_inc = 0;  // leave unchanged
        break;
        
        default: // should never happen
            std::cout << "Error: Illegal option for DMA 1 src adjust " << dma[1].src_adjust << "\n";
        break;
    }
    
    // 32 bit copy
    if (dma[1].chunk_size == 1)
    {
        for (int i = 0; i < dma[1].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write_u32(dest_ptr, read_u32(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u32);
            dest_ptr += dest_inc * sizeof(u32);
        }
    }

    // 16 bit copy
    else
    {
        for (int i = 0; i < dma[1].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write_u16(dest_ptr, read_u16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[1].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    write_u32_unprotected(REG_DMA3DAD, dest_ptr);

    // write back src
    write_u32_unprotected(REG_DMA3SAD, src_ptr);
    

    // turn off this transfer if repeat bit is not set
    if (dma[1].repeat == 0)
        dma[1].enable == 0;
    
    // IRQ request
    if (dma[1].irq)
        std::cout << "DMA1 IRQ request\n";
    
    std::cout << "DMA 1 Done\n";
}

void Memory::dma2()
{
    std::cout << "DMA 2\n";
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = read_u32_unprotected(REG_DMA3SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = read_u32_unprotected(REG_DMA3DAD) & 0x7FFFFFF; // 27 bit;

    // increment for destination, src
    int dest_inc, src_inc;

    // get increment mode for destination
    switch (dma[2].dest_adjust)
    {
        case 0:
            dest_inc = 1;  // increment after each copy
        break;

        case 1:
            dest_inc = -1; // decrement after each copy
        break;

        case 2:
            dest_inc = 0;  // leave unchanged
        break;

        case 3:
            dest_inc = 1;  // increment after each copy, reset after transfer
        break;
        
        default: // should never happen
            std::cout << "Error: Illegal option for DMA 2 destination adjust " << dma[2].dest_adjust << "\n";
        break;
    }
    
    // get increment mode for destination
    switch (dma[2].src_adjust)
    {
        case 0:
            src_inc = 1;  // increment after each copy
        break;

        case 1:
            src_inc = -1; // decrement after each copy
        break;

        case 2:
            src_inc = 0;  // leave unchanged
        break;
        
        default: // should never happen
            std::cout << "Error: Illegal option for DMA 2 src adjust " << dma[2].src_adjust << "\n";
        break;
    }
    
    // 32 bit copy
    if (dma[2].chunk_size == 1)
    {
        for (int i = 0; i < dma[2].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write_u32(dest_ptr, read_u32(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u32);
            dest_ptr += dest_inc * sizeof(u32);
        }
    }

    // 16 bit copy
    else
    {
        for (int i = 0; i < dma[2].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write_u16(dest_ptr, read_u16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[2].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    write_u32_unprotected(REG_DMA3DAD, dest_ptr);

    // write back src
    write_u32_unprotected(REG_DMA3SAD, src_ptr);
    

    // turn off this transfer if repeat bit is not set
    if (dma[2].repeat == 0)
        dma[2].enable == 0;
    
    // IRQ request
    if (dma[2].irq)
        std::cout << "DMA3 IRQ request\n";
    
    std::cout << "DMA 2 Done\n";
}

void Memory::dma3()
{
    std::cout << "DMA 3\n";
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = read_u32_unprotected(REG_DMA3SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = read_u32_unprotected(REG_DMA3DAD) & 0xFFFFFFF; // 28 bit;

    // increment for destination, src
    int dest_inc, src_inc;

    // get increment mode for destination
    switch (dma[3].dest_adjust)
    {
        case 0:
            dest_inc = 1;  // increment after each copy
        break;

        case 1:
            dest_inc = -1; // decrement after each copy
        break;

        case 2:
            dest_inc = 0;  // leave unchanged
        break;

        case 3:
            dest_inc = 1;  // increment after each copy, reset after transfer
        break;
        
        default: // should never happen
            std::cout << "Error: Illegal option for DMA 3 destination adjust " << dma[3].dest_adjust << "\n";
        break;
    }
    
    // get increment mode for destination
    switch (dma[3].src_adjust)
    {
        case 0:
            src_inc = 1;  // increment after each copy
        break;

        case 1:
            src_inc = -1; // decrement after each copy
        break;

        case 2:
            src_inc = 0;  // leave unchanged
        break;
        
        default: // should never happen
            std::cout << "Error: Illegal option for DMA 3 src adjust " << dma[3].src_adjust << "\n";
        break;
    }
    
    // 32 bit copy
    if (dma[3].chunk_size == 1)
    {
        for (int i = 0; i < dma[3].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write_u32(dest_ptr, read_u32(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u32);
            dest_ptr += dest_inc * sizeof(u32);
        }
    }

    // 16 bit copy
    else
    {
        for (int i = 0; i < dma[3].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write_u16(dest_ptr, read_u16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[3].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    write_u32_unprotected(REG_DMA3DAD, dest_ptr);

    // write back src
    write_u32_unprotected(REG_DMA3SAD, src_ptr);
    

    // turn off this transfer if repeat bit is not set
    if (dma[3].repeat == 0)
        dma[3].enable == 0;
    
    // IRQ request
    if (dma[3].irq)
        std::cout << "DMA3 IRQ request\n";   
    
    std::cout << "DMA 3 Done\n";
}