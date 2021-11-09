/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 *
 * FILE: Memory.cpp
 * DATE: July 13, 2020
 * DESCRIPTION: Implementation of memory related functions
 */
#include "Memory.h"
#include "IRQ.h"
#include "Flash.h"
#include "None.h"
#include "SRAM.h"
#include "config.h"
#include "util.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <experimental/filesystem>
#include <string.h>
#include <cassert>

extern IRQ *irq;

namespace fs = std::experimental::filesystem;

Memory::Memory(LcdStat *stat, Timer *timer, Gamepad *gamepad, AudioStat *audio_stat) :
    stat(stat),
    timer(timer),
    gamepad(gamepad),
    audio_stat(audio_stat)
{
    backup = nullptr;
    cart_ram = nullptr;

    reset();
}

Memory::~Memory()
{
    // dump cart ram contents to backup file
    backup->writeChip();    
}

void Memory::reset()
{
    // default cycle accesses for waitstates
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
}

bool Memory::loadRom(std::string const &name)
{
    std::ifstream rom(name, std::ios::in | std::ios::binary);

    if (!rom || !rom.good())
    {
        log(LogLevel::Error, "Error: Unable to open ROM file {}\n", name);
        exit(1);
    }

    rom_size = fs::file_size(name);

    rom.read((char *) cart_rom, rom_size);
    rom.close();

    // get cart RAM type
    std::string rom_temp((char *) cart_rom, rom_size);

    // eeprom
    if (rom_temp.find("EEPROM_V") != std::string::npos)
    {
        log(LogLevel::Warning, "Cart RAM EEPROM detected\n");
    }

    // flash 1M
    if (rom_temp.find("FLASH1M_V") != std::string::npos)
    {
        log(LogLevel::Warning, "Cart RAM FLASH128 detected\n");
        ram_size = 0x20000;
        backup   = new Flash(ram_size); 
    }

    // flash 512
    if (rom_temp.find("FLASH512_V") != std::string::npos)
    {
        log(LogLevel::Warning, "Cart RAM FLASH512 detected\n");
        ram_size = 0x10000;
        backup   = new Flash(ram_size); 
    }

    // flashv
    if (rom_temp.find("FLASH_V") != std::string::npos)
    {
        log(LogLevel::Warning, "Cart RAM FLASH detected\n");
        ram_size = 0x10000;
        backup   = new Flash(ram_size); 
    }

    // sram
    if (rom_temp.find("SRAM") != std::string::npos)
    {
        log(LogLevel::Warning, "Cart RAM SRAM detected\n");
        ram_size    = 0x10000;
        backup = new SRAM(0x10000);
    }

    // no cart RAM detected
    if (ram_size == 0)
    {
        log(LogLevel::Warning, "No cart RAM detected!\n");
        backup = new None(0x8000);
        return true;
    }

    // Load save file into cart RAM if one exists, otherwise create one
    config::backup_path = name + ".sav";
    if (util::pathExists(config::backup_path))
    {
        log("Save file {} detected. Loading now.\n", config::backup_path);
        backup->loadChip();
    }

    else
    {
        log("Save file {} not found. Creating now\n", config::backup_path);
        std::fstream backup { config::backup_path, std::ios::out };
        assert(backup);
        backup.close();
    }

    return true;
}

bool Memory::loadBios(std::string const &name)
{
    // bios must be called gba_bios.bin
    std::ifstream bios(name, std::ios::in | std::ios::binary);

    if (!bios || !bios.good())
    {
        log(LogLevel::Error, "Error: Unable to open BIOS file {}\n", name);
        exit(1);
    }

    bios.read((char *) memory, MEM_BIOS_SIZE);
    bios.close();
    return true;
}

u32 Memory::read32(u32 address)
{
    return (read8(address + 3) << 24)
    | (read8(address + 2) << 16)
    | (read8(address + 1) << 8)
    | read8(address);
}

u16 Memory::read16(u32 address)
{
    return (read8(address + 1) << 8) | read8(address);
}

u8 Memory::read8(u32 address)
{
    // get memory region for mirrors
    switch (address >> 24)
    {
        case 0x0: [[fallthrough]];
        case 0x1: [[fallthrough]];
        case 0x4: [[fallthrough]];
        case 0x8: [[fallthrough]];
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
            address &= 0xFFFF; // RAM Mirror
            return backup->read(address);

        default:
            std::cout << std::hex << address << "\n";
            assert(!"Error: Invalid address in Memory::read8");
            return 0;
    }


    // game rom
    if (address >= MEM_SIZE)
    {
        // if (address - MEM_SIZE > rom_size)
        //std::cout << "Caution: reading outside known cart length\n";

        return cart_rom[address - MEM_SIZE];
    }

    switch (address)
    {
        case REG_DISPSTAT:     return stat->dispstat.raw >> 0 & 0xFF;
        case REG_DISPSTAT + 1: return stat->dispstat.raw >> 8 & 0xFF;

        case REG_VCOUNT:       return stat->scanline;

        // REG_TM0D
        case REG_TM0D:         return timer->read(0) >> 0 & 0xFF;
        case REG_TM0D + 1:     return timer->read(0) >> 8 & 0xFF;

        // REG_TM1D
        case REG_TM1D:         return timer->read(1) >> 0 & 0xFF;
        case REG_TM1D + 1:     return timer->read(1) >> 8 & 0xFF;

        // REG_TM2D
        case REG_TM2D:         return timer->read(2) >> 0 & 0xFF;
        case REG_TM2D + 1:     return timer->read(2) >> 8 & 0xFF;

        // REG_TM3D
        case REG_TM3D:         return timer->read(3) >> 0 & 0xFF;
        case REG_TM3D + 1:     return timer->read(3) >> 8 & 0xFF;

        // REG_KEYINPUT
        case REG_KEYINPUT:     return gamepad->keys.raw >> 0 & 0xFF;
        case REG_KEYINPUT + 1: return gamepad->keys.raw >> 8 & 0xFF;

        // REG_IF
        case REG_IF:           return irq->getIF() >> 0 & 0xFF;
        case REG_IF + 1:       return irq->getIF() >> 8 & 0xFF;

        // REG_IE
        case REG_IE:           return irq->getIE() >> 0 & 0xFF;
        case REG_IE + 1:       return irq->getIE() >> 8 & 0xFF;

        // REG_IME
        case REG_IME:          return irq->getIME() >> 0 & 0xFF;
        case REG_IME + 1:      return irq->getIME() >> 8 & 0xFF;

        default:
            return memory[address];
    }
}

void Memory::write32(u32 address, u32 value)
{
    write8(address    , (value >>  0) & 0xFF);
    write8(address + 1, (value >>  8) & 0xFF);
    write8(address + 2, (value >> 16) & 0xFF);
    write8(address + 3, (value >> 24) & 0xFF);
}

void Memory::write16(u32 address, u16 value)
{
    write8(address    , (value >> 0) & 0xFF);
    write8(address + 1, (value >> 8) & 0xFF);
}

void Memory::write8(u32 address, u8 value)
{

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
            
            stat->oam_changed = true;
            
            // TODO: uncommenting this makes some nasty visual artifacts
            // if (!stat->dispcnt.hb && stat->dispstat.in_hBlank)
            //    return;

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
            address &= 0xFFFF; // RAM Mirror

            backup->write(address, value);
            return;

        default:
            log(LogLevel::Error, "Invalid address to write: 0x{x}\n", address);
            return;
    }

    // BIOS
    if (address <= 0x3FFF)
    {
        log(LogLevel::Error, "Error: Writing to BIOS\n");
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
        case REG_DISPCNT:   [[fallthrough]];
        case REG_DISPCNT + 1:
            stat->dispcnt.raw = (memory[REG_DISPCNT + 1] << 8) | (memory[REG_DISPCNT]);

            // bgs enabled
            stat->bgcnt[0].enabled = stat->dispcnt.bg_enabled & 0x1;
            stat->bgcnt[1].enabled = stat->dispcnt.bg_enabled & 0x2;
            stat->bgcnt[2].enabled = stat->dispcnt.bg_enabled & 0x4;
            stat->bgcnt[3].enabled = stat->dispcnt.bg_enabled & 0x8;
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
        case REG_BG0CNT:    [[fallthrough]];
        case REG_BG0CNT + 1:
            stat->bgcnt[0].raw = (memory[REG_BG0CNT + 1] << 8) | (memory[REG_BG0CNT]);
            // bit 13 (affine_wrap) is unused in BG0CNT
            stat->bgcnt[0].affine_wrap = 0;
            memory[REG_BG0CNT + 1] = stat->bgcnt[0].raw >> 8;
            memory[REG_BG0CNT] = stat->bgcnt[0].raw;
            break;

        // REG_BG1CNT
        case REG_BG1CNT:    [[fallthrough]];
        case REG_BG1CNT + 1:
            stat->bgcnt[1].raw = (memory[REG_BG1CNT + 1] << 8) | (memory[REG_BG1CNT]);
            // bit 13 (affine_wrap) is unused in BG1CNT
            stat->bgcnt[1].affine_wrap = 0;
            memory[REG_BG1CNT + 1] = stat->bgcnt[1].raw >> 8;
            memory[REG_BG1CNT] = stat->bgcnt[1].raw;
            break;

        // REG_BG2CNT
        case REG_BG2CNT:    [[fallthrough]];
        case REG_BG2CNT + 1:
            stat->bgcnt[2].raw = (memory[REG_BG2CNT + 1] << 8) | (memory[REG_BG2CNT]);
            break;

        // REG_BG3CNT
        case REG_BG3CNT:    [[fallthrough]];
        case REG_BG3CNT + 1:
            stat->bgcnt[3].raw = (memory[REG_BG3CNT + 1] << 8) | (memory[REG_BG3CNT]);
            break;

        // REG_BG0HOFS
        case REG_BG0HOFS:   [[fallthrough]];
        case REG_BG0HOFS + 1:
            stat->bgcnt[0].hoff = (memory[REG_BG0HOFS + 1] << 8) | (memory[REG_BG0HOFS]);
            break;

        // REG_BG0VOFS
        case REG_BG0VOFS:   [[fallthrough]];
        case REG_BG0VOFS + 1:
            stat->bgcnt[0].voff = (memory[REG_BG0VOFS + 1] << 8) | (memory[REG_BG0VOFS]);
            break;

        // REG_BG1HOFS
        case REG_BG1HOFS:   [[fallthrough]];
        case REG_BG1HOFS + 1:
            stat->bgcnt[1].hoff = (memory[REG_BG1HOFS + 1] << 8) | (memory[REG_BG1HOFS]);
            break;

        // REG_BG1VOFS
        case REG_BG1VOFS:   [[fallthrough]];
        case REG_BG1VOFS + 1:
            stat->bgcnt[1].voff = (memory[REG_BG1VOFS + 1] << 8) | (memory[REG_BG1VOFS]);
            break;

        // REG_BG2HOFS
        case REG_BG2HOFS:   [[fallthrough]];
        case REG_BG2HOFS + 1:
            stat->bgcnt[2].hoff = (memory[REG_BG2HOFS + 1] << 8) | (memory[REG_BG2HOFS]);
            break;

        // REG_BG2VOFS
        case REG_BG2VOFS:   [[fallthrough]];
        case REG_BG2VOFS + 1:
            stat->bgcnt[2].voff = (memory[REG_BG2VOFS + 1] << 8) | (memory[REG_BG2VOFS]);
            break;

        // REG_BG3HOFS
        case REG_BG3HOFS:   [[fallthrough]];
        case REG_BG3HOFS + 1:
            stat->bgcnt[3].hoff = (memory[REG_BG3HOFS + 1] << 8) | (memory[REG_BG3HOFS]);
            break;

        // REG_BG3VOFS
        case REG_BG3VOFS:   [[fallthrough]];
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
        case REG_BG2X + 0:  [[fallthrough]];
        case REG_BG2X + 1:  [[fallthrough]];
        case REG_BG2X + 2:  [[fallthrough]];
        case REG_BG2X + 3:
            stat->bgcnt[2].dx = (memory[REG_BG2X + 3] << 24) | (memory[REG_BG2X + 2] << 16) | (memory[REG_BG2X + 1] << 8) | (memory[REG_BG2X]);
            break;

        // REG_BG2Y
        case REG_BG2Y + 0:  [[fallthrough]];
        case REG_BG2Y + 1:  [[fallthrough]];
        case REG_BG2Y + 2:  [[fallthrough]];
        case REG_BG2Y + 3:
            stat->bgcnt[2].dy = (memory[REG_BG2Y + 3] << 24) | (memory[REG_BG2Y + 2] << 16) | (memory[REG_BG2Y + 1] << 8) | (memory[REG_BG2Y]);
            break;

        // REG_BG3X
        case REG_BG3X + 0:  [[fallthrough]];
        case REG_BG3X + 1:  [[fallthrough]];
        case REG_BG3X + 2:  [[fallthrough]];
        case REG_BG3X + 3:
            stat->bgcnt[3].dx = (memory[REG_BG3X + 3] << 24) | (memory[REG_BG3X + 2] << 16) | (memory[REG_BG3X + 1] << 8) | (memory[REG_BG3X]);
            break;

        // REG_BG3Y
        case REG_BG3Y + 0:  [[fallthrough]];
        case REG_BG3Y + 1:  [[fallthrough]];
        case REG_BG3Y + 2:  [[fallthrough]];
        case REG_BG3Y + 3:
            stat->bgcnt[3].dy = (memory[REG_BG3Y + 3] << 24) | (memory[REG_BG3Y + 2] << 16) | (memory[REG_BG3Y + 1] << 8) | (memory[REG_BG3Y]);
            break;
        
        // Window controls

        // REG_WIN0H
        case REG_WIN0H:     [[fallthrough]];
        case REG_WIN0H + 1:
            stat->writeWinh(0, memory[REG_WIN0H + 1] << 8 | memory[REG_WIN0H]);
            break;
        
        // REG_WIN0H
        case REG_WIN0V:     [[fallthrough]];
        case REG_WIN0V + 1:
            stat->writeWinv(0, memory[REG_WIN0V + 1] << 8 | memory[REG_WIN0V]);
            break;
        
        // REG_WIN1H
        case REG_WIN1H:     [[fallthrough]];
        case REG_WIN1H + 1:
            stat->writeWinh(1, memory[REG_WIN1H + 1] << 8 | memory[REG_WIN1H]);
            break;
        
        // REG_WIN1V
        case REG_WIN1V:     [[fallthrough]];
        case REG_WIN1V + 1:
            stat->writeWinv(1, memory[REG_WIN1V + 1] << 8 | memory[REG_WIN1V]);
            break;
        
        // REG_WININ
        case REG_WININ:
            // bits 6 - 7 not used
            value &= 0x3F;
            memory[address] = value;
            stat->writeWindowContent(CONTENT_WIN0, value);
            break;
        
        // REG_WININ + 1
        case REG_WININ + 1:
            // bits 13 - 14 not used
            value &= 0x3F;
            memory[address] = value;
            stat->writeWindowContent(CONTENT_WIN1, value);
            break;
        
        // REG_WINOUT
        case REG_WINOUT:
            // bits 6 - 7 not used
            value &= 0x3F;
            memory[address] = value;
            stat->writeWindowContent(CONTENT_WINOUT, value);
            break;
        
        // REG_WINOUT + 1
        case REG_WINOUT + 1:
            // bits 13 - 14 not used
            value &= 0x3F;
            memory[address] = value;
            stat->writeWindowContent(CONTENT_WINOBJ, value);
            break;

        // DMA

        // REG_DMA0CNT
        case REG_DMA0CNT:   [[fallthrough]];
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
                //log(LogLevel::Message, "DMA0 immediate\n");
                _dma(0);

                // disable DMA after immediate transfer
                dma[0].enable = 0;
            }

            break;

        // REG_DMA1CNT
        case REG_DMA1CNT:   [[fallthrough]];
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
                //log(LogLevel::Message, "DMA1 immediate\n");
                _dma(1);

                // disable DMA after immediate transfer
                dma[1].enable = 0;
            }

            break;

        // REG_DMA2CNT
        case REG_DMA2CNT:   [[fallthrough]];
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
                _dma(2);

                // disable DMA after immediate transfer
                dma[2].enable = 0;
            }

            break;

        // REG_DMA3CNT
        case REG_DMA3CNT:   [[fallthrough]];
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
                //log(LogLevel::Message, "DMA3 immediate\n");
                _dma(3);

                // disable DMA after immediate transfer
                dma[3].enable = 0;
            }

        break;

        // timers

        // REG_TM0D
        case REG_TM0D:  [[fallthrough]];
        case REG_TM0D + 1:
            timer->write(0, memory[REG_TM0D + 1] << 8 | memory[REG_TM0D]);
            break;
        
        // REG_TM1D
        case REG_TM1D:  [[fallthrough]];
        case REG_TM1D + 1:
            timer->write(1, memory[REG_TM1D + 1] << 8 | memory[REG_TM1D]);
            break;
        
        // REG_TM2D
        case REG_TM2D:  [[fallthrough]];
        case REG_TM2D + 1:
            timer->write(2, memory[REG_TM2D + 1] << 8 | memory[REG_TM2D]);
            break;
        
        // REG_TM3D
        case REG_TM3D:  [[fallthrough]];
        case REG_TM3D + 1:
            timer->write(3, memory[REG_TM3D + 1] << 8 | memory[REG_TM3D]);
            break;

        // REG_TM0CNT
        case REG_TM0CNT:
            timer->writeCnt(0, memory[REG_TM0CNT]);
            break;
        
        // REG_TM1CNT
        case REG_TM1CNT:
            timer->writeCnt(1, memory[REG_TM1CNT]);
            break;
        
        // REG_TM2CNT
        case REG_TM2CNT:
            timer->writeCnt(2, memory[REG_TM2CNT]);
            break;
        
        // REG_TM3CNT
        case REG_TM3CNT:
            timer->writeCnt(3, memory[REG_TM3CNT]);
            break;

        // REG_KEYCNT
        case REG_KEYCNT: [[fallthrough]];
        case REG_KEYCNT + 1:
            gamepad->keycnt.raw = memory[REG_KEYCNT + 1] << 8 | memory[REG_KEYCNT];
            break;
        
        // REG_IF
        case REG_IF:    [[fallthrough]];
        case REG_IF + 1:
            irq->clear(memory[REG_IF + 1] << 8 | memory[REG_IF]);
            break;
        
        // REG_IE
        case REG_IE:    [[fallthrough]];
        case REG_IE + 1:
            irq->setIE(memory[REG_IE + 1] << 8 | memory[REG_IE]);
            break;
        
        // REG_IME
        case REG_IME:    [[fallthrough]];
        case REG_IME + 1:
            irq->setIME(memory[REG_IME + 1] << 8 | memory[REG_IME]);
            break;
    }
}

u32 Memory::read32Unsafe(u32 address)
{
    return (read8Unsafe(address + 3) << 24)
    | (read8Unsafe(address + 2) << 16)
    | (read8Unsafe(address + 1) << 8)
    | read8Unsafe(address);
}

u16 Memory::read16Unsafe(u32 address)
{
    return (read8Unsafe(address + 1) << 8) | read8Unsafe(address);
}

u8 Memory::read8Unsafe(u32 address)
{
    return memory[address];
}

void Memory::write32Unsafe(u32 address, u32 value)
{
    write8Unsafe(address, value & 0xFF);
    write8Unsafe(address + 1, (value >> 8) & 0xFF);
    write8Unsafe(address + 2, (value >> 16) & 0xFF);
    write8Unsafe(address + 3, (value >> 24) & 0xFF);
}

void Memory::write16Unsafe(u32 address, u16 value)
{
    write8Unsafe(address, value & 0xFF);
    write8Unsafe(address + 1, (value >> 8) & 0xFF);
}

void Memory::write8Unsafe(u32 address, u8 value)
{
    memory[address] = value;
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
            log(LogLevel::Error, "Error: accessing unknown DMA: {}\n", n);
    }
}

void Memory::dma0()
{
    // log(LogLevel::Message, "DMA 0\n");
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = read32Unsafe(REG_DMA0SAD) & 0x7FFFFFF; // 27 bit
    dest_ptr = original_dest = read32Unsafe(REG_DMA0DAD) & 0x7FFFFFF; // 27 bit;

    // log(LogLevel::Debug, "DMA 0 start addr: 0x{x}\n", src_ptr);
    // log(LogLevel::Debug, "DMA 0 dest  addr: 0x{x}\n", dest_ptr);
    // log(LogLevel::Debug, "DMA 0 num transfers: {}\n", dma[0].num_transfers);

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
            log(LogLevel::Error, "Error: Illegal option for DMA 0 dest adjust: {}\n", (int) dma[0].dest_adjust);
            break;
    }

    // get increment mode for destination
    switch (dma[0].src_adjust)
    {
        case 0: src_inc =  1; break; // increment after each copy
        case 1: src_inc = -1; break; // decrement after each copy
        case 2: src_inc =  0; break; // leave unchanged
        default: // should never happen
            log(LogLevel::Error, "Error: Illegal option for DMA 0 src adjust: {}\n", (int) dma[0].src_adjust);
            break;
    }

    // 32 bit copy
    if (dma[0].chunk_size == 1)
    {
        for (int i = 0; i < dma[0].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write32(dest_ptr, read32(src_ptr));

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
            write16(dest_ptr, read16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[0].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    write32Unsafe(REG_DMA0DAD, dest_ptr);

    // write back src
    write32Unsafe(REG_DMA0SAD, src_ptr);


    // turn off this transfer if repeat bit is not set
    if (dma[0].repeat == 0)
        dma[0].enable = 0;

    // IRQ request
    if (dma[0].irq)
    {
        log(LogLevel::Debug, "DMA0 IRQ request\n");
        irq->raise(InterruptOccasion::DMA0);
    }

    //log(LogLevel::Debug, "DMA 0 Done\n");
}

void Memory::dma1()
{
    //log(LogLevel::Debug, "DMA 1\n");
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = read32Unsafe(REG_DMA1SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = read32Unsafe(REG_DMA1DAD) & 0x7FFFFFF; // 27 bit

    // log(LogLevel::Debug, "DMA 1 start addr: 0x{x}\n", src_ptr);
    // log(LogLevel::Debug, "DMA 1 dest  addr: 0x{x}\n", dest_ptr);
    // log(LogLevel::Debug, "DMA 1 num transfers: {}\n", dma[1].num_transfers);

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
            log(LogLevel::Error, "Error: Illegal option for DMA 1 dest adjust: {}\n", (int) dma[1].dest_adjust);
        break;
    }

    // get increment mode for destination
    switch (dma[1].src_adjust)
    {
        case 0: src_inc =  1; break; // increment after each copy
        case 1: src_inc = -1; break; // decrement after each copy
        case 2: src_inc =  0; break; // leave unchanged
        default: // should never happen
            log(LogLevel::Error, "Error: Illegal option for DMA 1 src adjust: {}\n", (int) dma[1].src_adjust);
            break;
    }

    // 32 bit copy
    if (dma[1].chunk_size == 1)
    {
        for (int i = 0; i < dma[1].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write32(dest_ptr, read32(src_ptr));

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
            write16(dest_ptr, read16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[1].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    write32Unsafe(REG_DMA1DAD, dest_ptr);

    // write back src
    write32Unsafe(REG_DMA1SAD, src_ptr);


    // turn off this transfer if repeat bit is not set
    if (dma[1].repeat == 0)
        dma[1].enable = 0;

    // IRQ request
    if (dma[1].irq)
    {
        log(LogLevel::Debug, "DMA1 IRQ request\n");
        irq->raise(InterruptOccasion::DMA1);
    }

    //log(LogLevel::Debug, "DMA 1 Done\n");
}

void Memory::dma2()
{
    //log(LogLevel::Debug, "DMA 2\n");
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = read32Unsafe(REG_DMA2SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = read32Unsafe(REG_DMA2DAD) & 0x7FFFFFF; // 27 bit;

    // log(LogLevel::Debug, "DMA 2 start addr: 0x{x}\n", src_ptr);
    // log(LogLevel::Debug, "DMA 2 dest  addr: 0x{x}\n", dest_ptr);
    // log(LogLevel::Debug, "DMA 2 num transfers: {}\n", dma[2].num_transfers);

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
            log(LogLevel::Error, "Error: Illegal option for DMA 2 dest adjust: {}\n", (int) dma[2].dest_adjust);
            break;
    }

    // get increment mode for destination
    switch (dma[2].src_adjust)
    {
        case 0: src_inc =  1; break; // increment after each copy
        case 1: src_inc = -1; break; // decrement after each copy
        case 2: src_inc =  0; break; // leave unchanged
        default: // should never happen
            log(LogLevel::Error, "Error: Illegal option for DMA 2 src adjust: {}\n", (int) dma[2].src_adjust);
            break;
    }

    // 32 bit copy
    if (dma[2].chunk_size == 1)
    {
        for (int i = 0; i < dma[2].num_transfers; ++i)
        {
            // copy memory from src address to dest address
            write32(dest_ptr, read32(src_ptr));

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
            write16(dest_ptr, read16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[2].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    write32Unsafe(REG_DMA2DAD, dest_ptr);

    // write back src
    write32Unsafe(REG_DMA2SAD, src_ptr);


    // turn off this transfer if repeat bit is not set
    if (dma[2].repeat == 0)
        dma[2].enable = 0;

    // IRQ request
    if (dma[2].irq)
    {
        log(LogLevel::Debug, "DMA2 IRQ request\n");
        irq->raise(InterruptOccasion::DMA2);
    }

    //log(LogLevel::Debug, "DMA 2 Done\n");
}

void Memory::dma3()
{
    //std::cout << "DMA 3\n";
    u32 dest_ptr, src_ptr, original_src, original_dest;
    src_ptr  = original_src  = read32Unsafe(REG_DMA3SAD) & 0xFFFFFFF; // 28 bit
    dest_ptr = original_dest = read32Unsafe(REG_DMA3DAD) & 0xFFFFFFF; // 28 bit;

    // increment for destination, src
    int dest_inc, src_inc;


    // log(LogLevel::Debug, "DMA 3 start addr: 0x{x}\n", (int) src_ptr);
    // log(LogLevel::Debug, "DMA 3 dest  addr: 0x{x}\n", (int) dest_ptr);
    // log(LogLevel::Debug, "DMA 3 num transfers: {}\n", dma[3].num_transfers);

    // get increment mode for destination
    switch (dma[3].dest_adjust)
    {
        case 0: dest_inc =  1; break;  // increment after each copy
        case 1: dest_inc = -1; break;  // decrement after each copy
        case 2: dest_inc =  0; break;  // leave unchanged
        case 3: dest_inc =  1; break;  // increment after each copy, reset after transfer
        default: // should never happen
            log(LogLevel::Error, "Error: Illegal option for DMA 3 dest adjust: {}\n", (int) dma[3].dest_adjust);
            break;
    }

    // get increment mode for src
    switch (dma[3].src_adjust)
    {
        case 0: src_inc =  1; break; // increment after each copy
        case 1: src_inc = -1; break; // decrement after each copy
        case 2: src_inc =  0; break; // leave unchanged
        default: // should never happen
            log(LogLevel::Error, "Error: Illegal option for DMA 3 src adjust: {}\n", (int) dma[3].src_adjust);
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
            write32(dest_ptr, read32(src_ptr));

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
            write16(dest_ptr, read16(src_ptr));

            // increment src, dest ptrs
            src_ptr  += src_inc  * sizeof(u16);
            dest_ptr += dest_inc * sizeof(u16);
        }
    }

    // reset initial destination address if required
    if (dma[3].dest_adjust == 3)
        dest_ptr = original_dest;

    // write back dest
    write32Unsafe(REG_DMA3DAD, dest_ptr);

    // write back src
    write32Unsafe(REG_DMA3SAD, src_ptr);


    // turn off this transfer if repeat bit is not set
    if (dma[3].repeat == 0)
        dma[3].enable = 0;

    // IRQ request
    if (dma[3].irq)
    {
        log(LogLevel::Debug, "DMA3 IRQ request\n");
        irq->raise(InterruptOccasion::DMA3);
    }
        
    //log(LogLevel::Debug, "DMA 3 Done\n");
}

Memory::Region Memory::getMemoryRegion(u32 address)
{
    switch (address >> 24)
    {
        case 0x0: return Region::BIOS;       
        case 0x2: return Region::EWRAM;
        case 0x3: return Region::IWRAM;
        case 0x4: return Region::MMIO;
        case 0x5: return Region::PALRAM;
        case 0x6: return Region::VRAM;
        case 0x7: return Region::OAM;
        case 0xA:
        case 0xB:
        case 0xC:
        case 0xD: return Region::ROM;
        case 0xF:
        case 0xE: return Region::RAM;
        case 0x1:
        case 0x8:
        case 0x9:
        default:
            return Region::UNKNOWN;
    }
}

// std::cout << "([a-zA-Z0-9 \\n]+)"
