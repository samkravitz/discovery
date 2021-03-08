/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 *
 * FILE: Memory.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of memory related functions
 */
#include <fstream>
#include <iostream>
#include <experimental/filesystem>
#include <string.h>

#include "Memory.h"

namespace fs = std::experimental::filesystem;

Memory::Memory(LcdStat *stat, Timer *timer, Gamepad *gamepad) :
    stat(stat),
    timer(timer),
    gamepad(gamepad)
{
    // cart_rom  = NULL;
    cart_ram  = NULL;

    Reset();
}

Memory::~Memory() { }

void Memory::Reset()
{
    // default cycle accesses for wait statae
    n_cycles = 4;
    s_cycles = 2;

    rom_size = 0;
    ram_size = 0;

    // zero memory
    for (int i = 0; i < MEM_SIZE; ++i)
        memory[i] = 0;

    for (int i = 0; i < 0x2000000; ++i)
        cart_rom[i] = 0;

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

    haltcnt = 0;

    // write all 1s to keypad (all keys cleared)
    Write32Unsafe(REG_KEYINPUT, 0b1111111111);
}

bool Memory::LoadRom(const std::string &name)
{
    std::ifstream rom(name, std::ios::in | std::ios::binary);

    if (!rom || !rom.good())
    {
        LOG(LogLevel::Error, "Error: Unable to open ROM file {}\n", name);
        exit(1);
    }

    rom_size = fs::file_size(name);

    //cart_rom = new u8[rom_size]();
    rom.read((char *) cart_rom, rom_size);
    rom.close();

    // get cart RAM type
    char *rom_temp = (char *) cart_rom;
    for (int i = 0; i < rom_size; ++i, ++rom_temp)
    {
        // FLASH RAM
        if (*rom_temp == 'F')
        {
            if (strncmp(rom_temp, "FLASH512_V", 10) == 0)
            {
                LOG(LogLevel::Message, "Cart RAM FLASH512 detected\n");

                ram_size = 0x10000;
                cart_ram = new u8[ram_size]();
            }

            if (strncmp(rom_temp, "FLASH1M_V", 8) == 0)
            {
                LOG(LogLevel::Message, "Cart RAM FLASH128 detected\n");

                ram_size = 0x20000;
                cart_ram = new u8[ram_size]();
            }

            if (strncmp(rom_temp, "FLASH_V", 7) == 0)
            {
                LOG(LogLevel::Message, "Cart RAM FLASH detected\n");

                ram_size = 0x10000;
                cart_ram = new u8[ram_size]();
            }
        }

        // SRAM
        if (*rom_temp == 'S')
        {
            if (strncmp(rom_temp, "SRAM_V", 6) == 0)
            {
                LOG(LogLevel::Message, "Cart RAM SRAM detected\n");

                ram_size = 0x8000;
                cart_ram = new u8[ram_size]();
            }
        }

        // EEPROM
        if (*rom_temp == 'E')
        {
            if (strncmp(rom_temp, "EEPROM_V", 8) == 0)
            {
                LOG(LogLevel::Message, "Cart RAM EEPROM detected\n");
            }
        }

        if (*rom_temp == 0xc2 && *(rom_temp + 1) == 0x32)
        {
            exit(32);
        }
    }

    // no cart RAM detected
    if (ram_size == 0)
        LOG(LogLevel::Warning, "No cart RAM detected!\n");

    return true;
}

bool Memory::LoadBios(const std::string &name)
{
    // bios must be called gba_bios.bin
    std::ifstream bios(name, std::ios::in | std::ios::binary);

    if (!bios || !bios.good())
    {
        LOG(LogLevel::Error, "Error: Unable to open BIOS file {}\n", name);
        exit(1);
    }

    bios.read((char *) memory, MEM_BIOS_SIZE);
    bios.close();
    return true;
}

u32 Memory::Read32(u32 address)
{
    return (Read8(address + 3) << 24)
    | (Read8(address + 2) << 16)
    | (Read8(address + 1) << 8)
    | Read8(address);
}

u16 Memory::Read16(u32 address)
{
    return (Read8(address + 1) << 8) | Read8(address);
}

u8 Memory::Read8(u32 address)
{
    if (address == 0xE000000)
    {
        return 0x62;
    }

    if (address == 0xE000001)
    {
        return 0x13;
    }
    // get memory region for mirrors
    switch (address >> 24)
    {
        case 0x0:
        case 0x1:
        case 0x4:
        case 0x8:
        case 0x9:
            break;

        // EWRAM
        case 0x2:
            address &= MEM_EWRAM_END;
            break;

        // IWRAM
        case 0x3:
            address &= MEM_IWRAM_END;
            break;

        // Palette RAM
        case 0x5:
            address &= MEM_PALETTE_RAM_END;
            break;

        // VRAM
        case 0x6:
            // 0x6010000 - 0x6017FFF is mirrored from 0x6018000 - 0x601FFFF.
            if (address >= 0x6018000 && address <= 0x601FFFF)
                address -= 0x8000;

            address &= 0x601FFFF;
            break;

        // OAM
        case 0x7:
            address &= MEM_OAM_END;
            break;

        // ROM image 1
        case 0xA:
        case 0xB:
            address -= 0x2000000;
            break;

        // ROM image 2
        case 0xC:
        case 0xD:
            address -= 0x4000000;
            break;

        // Cart RAM
        case 0xF:
            address -= 0x1000000;
        case 0xE:
            //std::cout << "Reading from cart RAM\n";
            address &= ~ram_size; // RAM Mirror
            return cart_ram[address - 0xE000000];

        default:
            LOG(LogLevel::Error, "Invalid address to read: 0x{x}\n", address);
            return 0;
    }


    // game rom
    if (address >= MEM_SIZE)
    {
        // if (address - MEM_SIZE > rom_size)
        //std::cout << "Caution: reading outside known cart length\n";

        return cart_rom[address - MEM_SIZE];
    }


    u8 result = 0;
    switch (address)
    {
        // IO reg
        case REG_DISPSTAT:
            result |= stat->displaystat.in_vBlank ? 0b1      : 0b0;       // bit 0 set in vblank, clear in vdraw
            result |= stat->displaystat.in_hBlank ? 0b10     : 0b00;      // bit 1 set in hblank, clear in hdraw
            result |= stat->displaystat.vcs       ? 0b100    : 0b000;     // bit 2
            result |= stat->displaystat.vbi       ? 0b1000   : 0b0000;    // bit 3
            result |= stat->displaystat.hbi       ? 0b10000  : 0b00000;   // bit 4
            result |= stat->displaystat.vci       ? 0b100000 : 0b000000;  // bit 5
            return result;

        case REG_DISPSTAT + 1:
            return stat->displaystat.vct;

        case REG_VCOUNT:
            return stat->scanline;

        // REG_TM0D
        case REG_TM0D    : return timer->Read(0) >> 0 & 0xFF;
        case REG_TM0D + 1: return timer->Read(0) >> 8 & 0xFF;

        // REG_TM1D
        case REG_TM1D    : return timer->Read(1) >> 0 & 0xFF;
        case REG_TM1D + 1: return timer->Read(1) >> 8 & 0xFF;

        // REG_TM2D
        case REG_TM2D    : return timer->Read(2) >> 0 & 0xFF;
        case REG_TM2D + 1: return timer->Read(2) >> 8 & 0xFF;

        // REG_TM3D
        case REG_TM3D    : return timer->Read(3) >> 0 & 0xFF;
        case REG_TM3D + 1: return timer->Read(3) >> 8 & 0xFF;

        // REG_KEYINPUT
        case REG_KEYINPUT    : return gamepad->keys.raw >> 0 & 0xFF;
        case REG_KEYINPUT + 1: return gamepad->keys.raw >> 8 & 0xFF;

        default:
            return memory[address];
    }
}

void Memory::Write32(u32 address, u32 value)
{
    Write8(address    , (value >>  0) & 0xFF);
    Write8(address + 1, (value >>  8) & 0xFF);
    Write8(address + 2, (value >> 16) & 0xFF);
    Write8(address + 3, (value >> 24) & 0xFF);
}

void Memory::Write16(u32 address, u16 value)
{
    Write8(address    , (value >> 0) & 0xFF);
    Write8(address + 1, (value >> 8) & 0xFF);
}

void Memory::Write8(u32 address, u8 value)
{

    if (address == REG_BG2X || address == REG_BG2Y)
    {
        //LOG("HI\n");
    }

    switch (address >> 24)
    {
        case 0x0:
        case 0x1:
        case 0x4:
        case 0x8:
        case 0x9:
            break;

        // EWRAM
        case 0x2:
            address &= MEM_EWRAM_END;
            break;

        // IWRAM
        case 0x3:
            address &= MEM_IWRAM_END;
            break;

        // Palette RAM
        case 0x5:
            address &= MEM_PALETTE_RAM_END;
            break;

        // VRAM
        case 0x6:
            // 0x6010000 - 0x6017FFF is mirrored from 0x6018000 - 0x601FFFF.
            if (address >= 0x6018000 && address <= 0x601FFFF)
                address -= 0x8000;

            address &= 0x601FFFF;
            break;

        // OAM
        case 0x7:
            address &= MEM_OAM_END;

            if (!stat->dispcnt.hb && stat->displaystat.in_hBlank)
                return;

            break;

        // ROM image 1
        case 0xA:
        case 0xB:
            address -= 0x2000000;
            break;

        // ROM image 2
        case 0xC:
        case 0xD:
            address -= 0x4000000;
            break;

        // Cart RAM
        case 0xF:
            address -= 0x1000000;
        case 0xE:
            //std::cout << "Writing to cart RAM\n";
            address &= ~ram_size; // RAM Mirror
            cart_ram[address - 0xE000000] = value;
            return;

        default:
            LOG(LogLevel::Error, "Invalid address to write: 0x{x}\n", address);
            return;
    }

    // BIOS
    if (address <= 0x3FFF)
    {
        LOG(LogLevel::Error, "Error: Writing to BIOS\n");
        return;
    }

    //game rom
    if (address >= MEM_SIZE)
    {
        //std::cerr << "Warning: writing to game rom\n";
        cart_rom[address - MEM_SIZE] = value;
        // std::cerr << "Done\n";
        return;
    }

    // write value at memory location
    memory[address] = value;

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
            stat->bgcnt[0].enabled             = value >> 0 & 0x1; // bit 8
            stat->bgcnt[1].enabled             = value >> 1 & 0x1; // bit 9
            stat->bgcnt[2].enabled             = value >> 2 & 0x1; // bit A
            stat->bgcnt[3].enabled             = value >> 3 & 0x1; // bit B
            stat->dispcnt.obj_enabled           = value >> 4 & 0x1; // bit C
            stat->dispcnt.win_enabled           = value >> 5 & 0x7; // bits D-F
        break;

        // REG_DISPSTAT
        case REG_DISPSTAT:
            // skip bits 0-2, unwritable
            stat->displaystat.vbi = value >> 3 & 1;
            stat->displaystat.hbi = value >> 4 & 1;
            stat->displaystat.vci = value >> 5 & 1;
        break;

        case REG_DISPSTAT + 1:
            stat->displaystat.vct = value;
        break;

        // REG_BG0CNT
        case REG_BG0CNT:
            stat->bgcnt[0].priority      = value >> 0 & 0x3; // bits 0-1
            stat->bgcnt[0].cbb           = value >> 2 & 0x3; // bits 2-3
            stat->bgcnt[0].mosaic        = value >> 6 & 0x1; // bit  6
            stat->bgcnt[0].color_mode    = value >> 7 & 0x1; // bit  7
        break;

        case REG_BG0CNT + 1:
            stat->bgcnt[0].sbb           = value >> 0 & 0x1F; // bits 8-C
            stat->bgcnt[0].affine_wrap   = value >> 5 & 0x1;  // bit  D
            stat->bgcnt[0].size          = value >> 6 & 0x3;  // bits E-F
        break;

        // REG_BG1CNT
        case REG_BG1CNT:
            stat->bgcnt[1].priority      = value >> 0 & 0x3; // bits 0-1
            stat->bgcnt[1].cbb           = value >> 2 & 0x3; // bits 2-3
            stat->bgcnt[1].mosaic        = value >> 6 & 0x1; // bit  6
            stat->bgcnt[1].color_mode    = value >> 7 & 0x1; // bit  7
        break;

        case REG_BG1CNT + 1:
            stat->bgcnt[1].sbb           = value >> 0 & 0x1F; // bits 8-C
            stat->bgcnt[1].affine_wrap   = value >> 5 & 0x1;  // bit  D
            stat->bgcnt[1].size          = value >> 6 & 0x3;  // bits E-F
        break;

        // REG_BG2CNT
        case REG_BG2CNT:
            stat->bgcnt[2].priority      = value >> 0 & 0x3; // bits 0-1
            stat->bgcnt[2].cbb           = value >> 2 & 0x3; // bits 2-3
            stat->bgcnt[2].mosaic        = value >> 6 & 0x1; // bit  6
            stat->bgcnt[2].color_mode    = value >> 7 & 0x1; // bit  7
        break;

        case REG_BG2CNT + 1:
            stat->bgcnt[2].sbb           = value >> 0 & 0x1F; // bits 8-C
            stat->bgcnt[2].affine_wrap   = value >> 5 & 0x1;  // bit  D
            stat->bgcnt[2].size          = value >> 6 & 0x3;  // bits E-F
        break;

        // REG_BG3CNT
        case REG_BG3CNT:
            stat->bgcnt[3].priority      = value >> 0 & 0x3; // bits 0-1
            stat->bgcnt[3].cbb           = value >> 2 & 0x3; // bits 2-3
            stat->bgcnt[3].mosaic        = value >> 6 & 0x1; // bit  6
            stat->bgcnt[3].color_mode    = value >> 7 & 0x1; // bit  7
        break;

        case REG_BG3CNT + 1:
            stat->bgcnt[3].sbb           = value >> 0 & 0x1F; // bits 8-C
            stat->bgcnt[3].affine_wrap   = value >> 5 & 0x1;  // bit  D
            stat->bgcnt[3].size          = value >> 6 & 0x3;  // bits E-F
        break;

        // REG_BG0HOFS
        case REG_BG0HOFS:
        case REG_BG0HOFS + 1:
            stat->bgcnt[0].hoff = (memory[REG_BG0HOFS + 1] << 8) | (memory[REG_BG0HOFS]);
            break;

        // REG_BG0VOFS
        case REG_BG0VOFS:
        case REG_BG0VOFS + 1:
            stat->bgcnt[0].voff = (memory[REG_BG0VOFS + 1] << 8) | (memory[REG_BG0VOFS]);
            break;

        // REG_BG1HOFS
        case REG_BG1HOFS:
        case REG_BG1HOFS + 1:
            stat->bgcnt[1].hoff = (memory[REG_BG1HOFS + 1] << 8) | (memory[REG_BG1HOFS]);
            break;

        // REG_BG1VOFS
        case REG_BG1VOFS:
        case REG_BG1VOFS + 1:
            stat->bgcnt[1].voff = (memory[REG_BG1VOFS + 1] << 8) | (memory[REG_BG1VOFS]);
            break;

        // REG_BG2HOFS
        case REG_BG2HOFS:
        case REG_BG2HOFS + 1:
            stat->bgcnt[2].hoff = (memory[REG_BG2HOFS + 1] << 8) | (memory[REG_BG2HOFS]);
            break;

        // REG_BG2VOFS
        case REG_BG2VOFS:
        case REG_BG2VOFS + 1:
            stat->bgcnt[2].voff = (memory[REG_BG2VOFS + 1] << 8) | (memory[REG_BG2VOFS]);
            break;

        // REG_BG3HOFS
        case REG_BG3HOFS:
        case REG_BG3HOFS + 1:
            stat->bgcnt[3].hoff = (memory[REG_BG3HOFS + 1] << 8) | (memory[REG_BG3HOFS]);
            break;

        // REG_BG3VOFS
        case REG_BG3VOFS:
        case REG_BG3VOFS + 1:
            stat->bgcnt[3].voff = (memory[REG_BG3VOFS + 1] << 8) | (memory[REG_BG3VOFS]);
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

        // REG_BG2X
        case REG_BG2X + 0:
        case REG_BG2X + 1:
        case REG_BG2X + 2:
        case REG_BG2X + 3:
            stat->bgcnt[2].dx = (memory[REG_BG2X + 3] << 24) | (memory[REG_BG2X + 2] << 16) | (memory[REG_BG2X + 1] << 8) | (memory[REG_BG2X]);
            break;

        // REG_BG2Y
        case REG_BG2Y + 0:
        case REG_BG2Y + 1:
        case REG_BG2Y + 2:
        case REG_BG2Y + 3:
            stat->bgcnt[2].dy = (memory[REG_BG2Y + 3] << 24) | (memory[REG_BG2Y + 2] << 16) | (memory[REG_BG2Y + 1] << 8) | (memory[REG_BG2Y]);
            break;

        // REG_BG3X
        case REG_BG3X + 0:
        case REG_BG3X + 1:
        case REG_BG3X + 2:
        case REG_BG3X + 3:
            stat->bgcnt[3].dx = (memory[REG_BG3X + 3] << 24) | (memory[REG_BG3X + 2] << 16) | (memory[REG_BG3X + 1] << 8) | (memory[REG_BG3X]);
            break;

        // REG_BG3Y
        case REG_BG3Y + 0:
        case REG_BG3Y + 1:
        case REG_BG3Y + 2:
        case REG_BG3Y + 3:
            stat->bgcnt[3].dy = (memory[REG_BG3Y + 3] << 24) | (memory[REG_BG3Y + 2] << 16) | (memory[REG_BG3Y + 1] << 8) | (memory[REG_BG3Y]);
            break;

        // DMA

        // REG_DMA0CNT
        case REG_DMA0CNT:
        case REG_DMA0CNT + 1:
            dma[0].num_transfers = (memory[REG_DMA0CNT + 1] << 8) | (memory[REG_DMA0CNT]);
            break;

       case REG_DMA0CNT + 2:
            dma[0].dest_adjust   = value >> 5 & 0x3;
            break;

        case REG_DMA0CNT + 3:
            dma[0].src_adjust     = ((memory[REG_DMA0CNT + 3] & 1) << 1) | (memory[REG_DMA0CNT + 2] >> 7);
            dma[0].repeat         = value >> 1 & 0x1;
            dma[0].chunk_size     = value >> 2 & 0x1;
            dma[0].mode           = value >> 4 & 0x3;
            dma[0].irq            = value >> 6 & 0x1;
            dma[0].enable         = value >> 7 & 0x1;

            if (dma[0].enable && dma[0].mode == 0) // immediate mode
            {
                //LOG(LogLevel::Message, "DMA0 immediate\n");
                _Dma(0);

                // disable DMA after immediate transfer
                dma[0].enable = 0;
            }

            break;

        // REG_DMA1CNT
        case REG_DMA1CNT:
        case REG_DMA1CNT + 1:
            dma[1].num_transfers = (memory[REG_DMA1CNT + 1] << 8) | (memory[REG_DMA1CNT]);
            break;

        case REG_DMA1CNT + 2:
            dma[1].dest_adjust   = value >> 5 & 0x3;
            break;

        case REG_DMA1CNT + 3:
            dma[1].src_adjust     = ((memory[REG_DMA1CNT + 3] & 1) << 1) | (memory[REG_DMA1CNT + 2] >> 7);
            dma[1].repeat         = value >> 1 & 0x1;
            dma[1].chunk_size     = value >> 2 & 0x1;
            dma[1].mode           = value >> 4 & 0x3;
            dma[1].irq            = value >> 6 & 0x1;
            dma[1].enable         = value >> 7 & 0x1;

            if (dma[1].enable && dma[1].mode == 0) // immediate mode
            {
                //LOG(LogLevel::Message, "DMA1 immediate\n");
                _Dma(1);

                // disable DMA after immediate transfer
                dma[1].enable = 0;
            }

            break;

        // REG_DMA2CNT
        case REG_DMA2CNT:
        case REG_DMA2CNT + 1:
            dma[2].num_transfers = (memory[REG_DMA2CNT + 1] << 8) | (memory[REG_DMA2CNT]);
            break;

        case REG_DMA2CNT + 2:
            dma[2].dest_adjust   = value >> 5 & 0x3;
            break;

        case REG_DMA2CNT + 3:
            dma[2].src_adjust     = ((memory[REG_DMA2CNT + 3] & 1) << 1) | (memory[REG_DMA2CNT + 2] >> 7);
            dma[2].repeat         = value >> 1 & 0x1;
            dma[2].chunk_size     = value >> 2 & 0x1;
            dma[2].mode           = value >> 4 & 0x3;
            dma[2].irq            = value >> 6 & 0x1;
            dma[2].enable         = value >> 7 & 0x1;

            if (dma[2].enable && dma[2].mode == 0) // immediate mode
            {
                //(LogLevel::Message, "DMA2 immediate\n");
                _Dma(2);

                // disable DMA after immediate transfer
                dma[2].enable = 0;
            }

            break;

        // REG_DMA3CNT
        case REG_DMA3CNT:
        case REG_DMA3CNT + 1:
            dma[3].num_transfers = (memory[REG_DMA3CNT + 1] << 8) | (memory[REG_DMA3CNT]);
            break;

        case REG_DMA3CNT + 2:
            dma[3].dest_adjust   = value >> 5 & 0x3;
            break;

        case REG_DMA3CNT + 3:
            dma[3].src_adjust     = ((memory[REG_DMA3CNT + 3] & 1) << 1) | (memory[REG_DMA3CNT + 2] >> 7);
            dma[3].repeat         = value >> 1 & 0x1;
            dma[3].chunk_size     = value >> 2 & 0x1;
            dma[3].mode           = value >> 4 & 0x3;
            dma[3].irq            = value >> 6 & 0x1;
            dma[3].enable         = value >> 7 & 0x1;

            if (dma[3].enable && dma[3].mode == 0) // immediate mode
            {
                //LOG(LogLevel::Message, "DMA3 immediate\n");
                _Dma(3);

                // disable DMA after immediate transfer
                dma[3].enable = 0;
            }

        break;

        // timers

        // REG_TM0D
        case REG_TM0D:
        case REG_TM0D + 1:
            timer->Write(0, memory[REG_TM0D + 1] << 8 | memory[REG_TM0D]);
            break;
        
        // REG_TM1D
        case REG_TM1D:
        case REG_TM1D + 1:
            timer->Write(1, memory[REG_TM1D + 1] << 8 | memory[REG_TM1D]);
            break;
        
        // REG_TM2D
        case REG_TM2D:
        case REG_TM2D + 1:
            timer->Write(2, memory[REG_TM2D + 1] << 8 | memory[REG_TM2D]);
            break;
        
        // REG_TM3D
        case REG_TM3D:
        case REG_TM3D + 1:
            timer->Write(3, memory[REG_TM3D + 1] << 8 | memory[REG_TM3D]);
            break;

        // REG_TM0CNT
        case REG_TM0CNT:
            timer->WriteCnt(0, memory[REG_TM0CNT]);
            break;
        
        // REG_TM1CNT
        case REG_TM1CNT:
            timer->WriteCnt(1, memory[REG_TM1CNT]);
            break;
        
        // REG_TM2CNT
        case REG_TM2CNT:
            timer->WriteCnt(2, memory[REG_TM2CNT]);
            break;
        
        // REG_TM3CNT
        case REG_TM3CNT:
            timer->WriteCnt(3, memory[REG_TM3CNT]);
            break;
        
        // REG_IF
        case REG_IF:
        case REG_IF + 1:
            memory[address] &= ~value;
            break;
        
        case REG_HALTCNT:
            haltcnt = 1;
            break;
    }
}

u32 Memory::Read32Unsafe(u32 address)
{
    return (Read8Unsafe(address + 3) << 24)
    | (Read8Unsafe(address + 2) << 16)
    | (Read8Unsafe(address + 1) << 8)
    | Read8Unsafe(address);
}

u16 Memory::Read16Unsafe(u32 address)
{
    return (Read8Unsafe(address + 1) << 8) | Read8Unsafe(address);
}

u8 Memory::Read8Unsafe(u32 address)
{
    return memory[address];
}

void Memory::Write32Unsafe(u32 address, u32 value)
{
    Write8Unsafe(address, value & 0xFF);
    Write8Unsafe(address + 1, (value >> 8) & 0xFF);
    Write8Unsafe(address + 2, (value >> 16) & 0xFF);
    Write8Unsafe(address + 3, (value >> 24) & 0xFF);
}

void Memory::Write16Unsafe(u32 address, u16 value)
{
    Write8Unsafe(address, value & 0xFF);
    Write8Unsafe(address + 1, (value >> 8) & 0xFF);
}

void Memory::Write8Unsafe(u32 address, u8 value)
{
    memory[address] = value;
}

void Memory::_Dma(int n)
{
    switch (n)
    {
        case 0: Dma0(); break;
        case 1: Dma1(); break;
        case 2: Dma2(); break;
        case 3: Dma3(); break;

        default: // should never happen
            LOG(LogLevel::Error, "Error: accessing unknown DMA: {}\n", n);
    }
}

void Memory::Dma0()
{
    LOG(LogLevel::Message, "DMA 0\n");
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = Read32Unsafe(REG_DMA0SAD) & 0x7FFFFFF; // 27 bit
    dest_ptr = original_dest = Read32Unsafe(REG_DMA0DAD) & 0x7FFFFFF; // 27 bit;

    // LOG(LogLevel::Debug, "DMA 0 start addr: 0x{x}\n", src_ptr);
    // LOG(LogLevel::Debug, "DMA 0 dest  addr: 0x{x}\n", dest_ptr);
    // LOG(LogLevel::Debug, "DMA 0 num transfers: {}\n", dma[0].num_transfers);

    // increment for destination, src
    int dest_inc, src_inc;

    // get increment mode for destination
    switch (dma[0].dest_adjust)
    {
        case 0: dest_inc =  1; break;  // increment after each copy
        case 1: dest_inc = -1; break;  // decrement after each copy
        case 2: dest_inc =  0; break;  // leave unchanged
        case 3: dest_inc =  1; break;  // increment after each copy, reset after transfer
        default: // should never happen
            LOG(LogLevel::Error, "Error: Illegal option for DMA 0 dest adjust: {}\n", (int) dma[0].dest_adjust);
            break;
    }

    // get increment mode for destination
    switch (dma[0].src_adjust)
    {
        case 0: src_inc =  1; break; // increment after each copy
        case 1: src_inc = -1; break; // decrement after each copy
        case 2: src_inc =  0; break; // leave unchanged
        default: // should never happen
            LOG(LogLevel::Error, "Error: Illegal option for DMA 0 src adjust: {}\n", (int) dma[0].src_adjust);
            break;
    }

    // 32 bit copy
    if (dma[0].chunk_size == 1)
    {
        for (int i = 0; i < dma[0].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            Write32(dest_ptr, Read32(src_ptr));

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
            Write16(dest_ptr, Read16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[0].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    Write32Unsafe(REG_DMA0DAD, dest_ptr);

    // write back src
    Write32Unsafe(REG_DMA0SAD, src_ptr);


    // turn off this transfer if repeat bit is not set
    if (dma[0].repeat == 0)
        dma[0].enable = 0;

    // IRQ request
    if (dma[0].irq)
        LOG(LogLevel::Debug, "DMA0 IRQ request\n");

    //LOG(LogLevel::Debug, "DMA 0 Done\n");
}

void Memory::Dma1()
{
    //LOG(LogLevel::Debug, "DMA 1\n");
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = Read32Unsafe(REG_DMA1SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = Read32Unsafe(REG_DMA1DAD) & 0x7FFFFFF; // 27 bit

    // LOG(LogLevel::Debug, "DMA 1 start addr: 0x{x}\n", src_ptr);
    // LOG(LogLevel::Debug, "DMA 1 dest  addr: 0x{x}\n", dest_ptr);
    // LOG(LogLevel::Debug, "DMA 1 num transfers: {}\n", dma[1].num_transfers);

    // increment for destination, src
    int dest_inc, src_inc;

    // get increment mode for destination
    switch (dma[1].dest_adjust)
    {
        case 0: dest_inc =  1; break;  // increment after each copy
        case 1: dest_inc = -1; break;  // decrement after each copy
        case 2: dest_inc =  0; break;  // leave unchanged
        case 3: dest_inc =  1; break;  // increment after each copy, reset after transfer
        default: // should never happen
            LOG(LogLevel::Error, "Error: Illegal option for DMA 1 dest adjust: {}\n", (int) dma[1].dest_adjust);
        break;
    }

    // get increment mode for destination
    switch (dma[1].src_adjust)
    {
        case 0: src_inc =  1; break; // increment after each copy
        case 1: src_inc = -1; break; // decrement after each copy
        case 2: src_inc =  0; break; // leave unchanged
        default: // should never happen
            LOG(LogLevel::Error, "Error: Illegal option for DMA 1 src adjust: {}\n", (int) dma[1].src_adjust);
            break;
    }

    // 32 bit copy
    if (dma[1].chunk_size == 1)
    {
        for (int i = 0; i < dma[1].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            Write32(dest_ptr, Read32(src_ptr));

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
            Write16(dest_ptr, Read16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[1].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    Write32Unsafe(REG_DMA1DAD, dest_ptr);

    // write back src
    Write32Unsafe(REG_DMA1SAD, src_ptr);


    // turn off this transfer if repeat bit is not set
    if (dma[1].repeat == 0)
        dma[1].enable = 0;

    // IRQ request
    if (dma[1].irq)
        LOG(LogLevel::Debug, "DMA1 IRQ request\n");

    //LOG(LogLevel::Debug, "DMA 1 Done\n");
}

void Memory::Dma2()
{
    //LOG(LogLevel::Debug, "DMA 2\n");
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = Read32Unsafe(REG_DMA2SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = Read32Unsafe(REG_DMA2DAD) & 0x7FFFFFF; // 27 bit;

    // LOG(LogLevel::Debug, "DMA 2 start addr: 0x{x}\n", src_ptr);
    // LOG(LogLevel::Debug, "DMA 2 dest  addr: 0x{x}\n", dest_ptr);
    // LOG(LogLevel::Debug, "DMA 2 num transfers: {}\n", dma[2].num_transfers);

    // increment for destination, src
    int dest_inc, src_inc;

    // get increment mode for destination
    switch (dma[2].dest_adjust)
    {
        case 0: dest_inc =  1; break;  // increment after each copy
        case 1: dest_inc = -1; break;  // decrement after each copy
        case 2: dest_inc =  0; break;  // leave unchanged
        case 3: dest_inc =  1; break;  // increment after each copy, reset after transfer
        default: // should never happen
            LOG(LogLevel::Error, "Error: Illegal option for DMA 2 dest adjust: {}\n", (int) dma[2].dest_adjust);
            break;
    }

    // get increment mode for destination
    switch (dma[2].src_adjust)
    {
        case 0: src_inc =  1; break; // increment after each copy
        case 1: src_inc = -1; break; // decrement after each copy
        case 2: src_inc =  0; break; // leave unchanged
        default: // should never happen
            LOG(LogLevel::Error, "Error: Illegal option for DMA 2 src adjust: {}\n", (int) dma[2].src_adjust);
            break;
    }

    // 32 bit copy
    if (dma[2].chunk_size == 1)
    {
        for (int i = 0; i < dma[2].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            Write32(dest_ptr, Read32(src_ptr));

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
            Write16(dest_ptr, Read16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[2].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    Write32Unsafe(REG_DMA2DAD, dest_ptr);

    // write back src
    Write32Unsafe(REG_DMA2SAD, src_ptr);


    // turn off this transfer if repeat bit is not set
    if (dma[2].repeat == 0)
        dma[2].enable = 0;

    // IRQ request
    if (dma[2].irq)
        LOG(LogLevel::Debug, "DMA3 IRQ request\n");

    //LOG(LogLevel::Debug, "DMA 2 Done\n");
}

void Memory::Dma3()
{
    //std::cout << "DMA 3\n";
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = Read32Unsafe(REG_DMA3SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = Read32Unsafe(REG_DMA3DAD) & 0xFFFFFFF; // 28 bit;

    // increment for destination, src
    int dest_inc, src_inc;

    // LOG(LogLevel::Debug, "DMA 3 start addr: 0x{x}\n", (int) src_ptr);
    // LOG(LogLevel::Debug, "DMA 3 dest  addr: 0x{x}\n", (int) dest_ptr);
    // LOG(LogLevel::Debug, "DMA 3 num transfers: {}\n", dma[3].num_transfers);

    // get increment mode for destination
    switch (dma[3].dest_adjust)
    {
        case 0: dest_inc =  1; break;  // increment after each copy
        case 1: dest_inc = -1; break;  // decrement after each copy
        case 2: dest_inc =  0; break;  // leave unchanged
        case 3: dest_inc =  1; break;  // increment after each copy, reset after transfer
        default: // should never happen
            LOG(LogLevel::Error, "Error: Illegal option for DMA 3 dest adjust: {}\n", (int) dma[3].dest_adjust);
            break;
    }

    // get increment mode for src
    switch (dma[3].src_adjust)
    {
        case 0: src_inc =  1; break; // increment after each copy
        case 1: src_inc = -1; break; // decrement after each copy
        case 2: src_inc =  0; break; // leave unchanged
        default: // should never happen
            LOG(LogLevel::Error, "Error: Illegal option for DMA 3 src adjust: {}\n", (int) dma[3].src_adjust);
            break;
    }

    // 32 bit copy
    if (dma[3].chunk_size == 1)
    {
        src_ptr  &= ~0x3; original_src  = src_ptr;
        dest_ptr &= ~0x3; original_dest = dest_ptr;
        for (int i = 0; i < dma[3].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            Write32(dest_ptr, Read32(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u32);
            dest_ptr += dest_inc * sizeof(u32);
        }
    }

    // 16 bit copy
    else
    {
        src_ptr  &= ~0x1; original_src  = src_ptr;
        dest_ptr &= ~0x1; original_dest = dest_ptr;

        for (int i = 0; i < dma[3].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            Write16(dest_ptr, Read16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[3].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    Write32Unsafe(REG_DMA3DAD, dest_ptr);

    // write back src
    Write32Unsafe(REG_DMA3SAD, src_ptr);


    // turn off this transfer if repeat bit is not set
    if (dma[3].repeat == 0)
        dma[3].enable = 0;

    // IRQ request
    if (dma[3].irq)
        LOG(LogLevel::Debug, "DMA3 IRQ request\n");

    //LOG(LogLevel::Debug, "DMA 3 Done\n");
}

// std::cout << "([a-zA-Z0-9 \\n]+)"
