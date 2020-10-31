/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: memory.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of memory related functions
 */
#include "memory.h"

#include <fstream>
#include <iostream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

Memory::Memory()
{
    game_rom = NULL;
    stat = NULL;
    reset();
}

Memory::~Memory() { }

void Memory::reset()
{
    // default cycle accesses for wait statae
    n_cycles = 4;
    s_cycles = 2;

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
            result |= stat->in_vBlank ? 0b1  : 0b0;  // bit 0 set in vblank, clear in vdraw
            result |= stat->in_hBlank ? 0b10 : 0b00; // bit 1 set in hblank, clear in hdraw
            //std::cout << "Polling REG_DISPSTAT " << (int) result << "\n";
            return result;
        case REG_VCOUNT:
            return stat->current_scanline;

        default:
            return memory[address];
    }
}

void Memory::write_u32(u32 address, u32 value)
{
    // DMA
    switch (address)
    {
        // REG_DMAxSAD
        case REG_DMA0SAD:
        case REG_DMA1SAD:
        case REG_DMA2SAD:
        case REG_DMA3SAD:
        {
            int i;
            switch (address & 0xF)
            {
                case 0x0: i = 0; break;
                case 0xC: i = 1; break;
                case 0x8: i = 2; break;
                case 0x4: i = 3; break;
            }

            dma[i].src_address = value;
        }
        break;

        // REG_DMAxDAD
        case REG_DMA0DAD:
        case REG_DMA1DAD:
        case REG_DMA2DAD:
        case REG_DMA3DAD:
        {
            int i;
            switch (address & 0xF)
            {
                case 0x4: i = 0; break;
                case 0x0: i = 1; break;
                case 0xC: i = 2; break;
                case 0x8: i = 3; break;
            }

            dma[i].dest_address = value;
        }
        break;

        // REG_DMAxCNT
        case REG_DMA0CNT:
        case REG_DMA1CNT:
        case REG_DMA2CNT:
        case REG_DMA3CNT:
        {
            int i;
            switch (address & 0xF)
            {
                case 0x8: i = 0; break;
                case 0x4: i = 1; break;
                case 0x0: i = 2; break;
                case 0xC: i = 3; break;
            }

            dma[i].num_transfers    = (value >>  0) & 0xFFFF; // bits 15 - 0
            dma[i].dest_adjust      = (value >> 21) &    0x3; // bits 22 - 21
            dma[i].src_adjust       = (value >> 23) &    0x3; // bits 24 - 23
            dma[i].repeat           = (value >> 25) &    0x1; // bit  25
            dma[i].chunk_size       = (value >> 26) &    0x1; // bit  26
            dma[i].mode             = (value >> 28) &    0x3; // bits 29 - 28
            dma[i].irq              = (value >> 30) &    0x1; // bit  30
            dma[i].enable           = (value >> 31) &    0x1; // bit  31

            // immediate transfer timing
            if (dma[i].enable && dma[i].mode == 0)
                _dma(i);
        }
        break;

    }
    
    // if (address >= REG_DMA0SAD && address <= REG_DMA3CNT)
    //     std::cout << "DMA OP\n";
    write_u8(address    , (value >>  0) & 0xFF);
    write_u8(address + 1, (value >>  8) & 0xFF);
    write_u8(address + 2, (value >> 16) & 0xFF);
    write_u8(address + 3, (value >> 24) & 0xFF);
}

void Memory::write_u16(u32 address, u16 value)
{
    write_u8(address, value & 0xFF);
    write_u8(address + 1, (value >> 8) & 0xFF);
}

// TODO - add protection against VRAM byte writes
void Memory::write_u8(u32 address, u8 value)
{
    // game rom
    if (address >= MEM_SIZE)
    {
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
            stat->reg_dispcnt.mode                  = value >> 0 & 0x7; // bits 0-2     
            stat->reg_dispcnt.gb                    = value >> 3 & 0x1; // bit 3
            stat->reg_dispcnt.ps                    = value >> 4 & 0x1; // bit 4
            stat->reg_dispcnt.hb                    = value >> 5 & 0x1; // bit 5
            stat->reg_dispcnt.obj_map_mode          = value >> 6 & 0x1; // bit 6
            stat->reg_dispcnt.fb                    = value >> 7 & 0x1; // bit 7    
        break;

        case REG_DISPCNT + 1:
            stat->bg_cnt[0].enabled                 = value >> 0 & 0x1; // bit 8
            stat->bg_cnt[1].enabled                 = value >> 1 & 0x1; // bit 9
            stat->bg_cnt[2].enabled                 = value >> 2 & 0x1; // bit A
            stat->bg_cnt[3].enabled                 = value >> 3 & 0x1; // bit B
            stat->reg_dispcnt.obj_enabled           = value >> 4 & 0x1; // bit C
            stat->reg_dispcnt.win_enabled           = value >> 5 & 0x7; // bits D-F
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
    }

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

// TODO - add protection against VRAM byte writes
void Memory::write_u8_unprotected(u32 address, u8 value)
{
    memory[address] = value;
}

void Memory::load_rom(char *name)
{
    std::ifstream rom(name, std::ios::in | std::ios::binary);

    if (!rom)
        return;
    size_t size = fs::file_size(name);

    if (!rom.good())
    {
        std::cerr << "Bad rom!" << "\n";
        return;
    }

    game_rom = new u8[size]();
    rom.read((char *) game_rom, size);
    rom.close();
}

void Memory::load_bios()
{
    // bios must be called gba_bios.bin
    std::ifstream bios("gba_bios.bin", std::ios::in | std::ios::binary);
    if (!bios)
        return;

    if (!bios.good())
    {
        std::cerr << "Bad bios!" << "\n";
        return;
    }

    bios.read((char *) memory, MEM_BIOS_SIZE);
    bios.close();
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
}

void Memory::dma1()
{
    std::cout << "DMA 1\n";   
}

void Memory::dma2()
{
    std::cout << "DMA 2\n";   
}

void Memory::dma3()
{
    std::cout << "DMA 3\n";  
}