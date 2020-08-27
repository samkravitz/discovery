/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: common.h
 * DATE: July 11, 2020
 * DESCRIPTION: common values and types for gba memory
 */
#ifndef MEM_COMMON_H
#define MEM_COMMON_H


// start and end addresses of internal memory regions
#define MEM_BIOS_END          0x3FFF
#define MEM_BOARD_WRAM_START  0x2000000
#define MEM_BOARD_WRAM_END    0x203FFFF
#define MEM_CHIP_WRAM_START   0x3000000
#define MEM_CHIP_WRAM_END     0x3007FFF
#define MEM_IO_REG_START      0x4000000
#define MEM_IO_REG_END        0x40003FE
#define MEM_PALETTE_RAM_START 0x5000000
#define MEM_PALLETTE_RAM_END  0x50003FF
#define MEM_VRAM_START        0x6000000
#define MEM_VRAM_END          0x6017FFF
#define MEM_OAM_START         0x7000000
#define MEM_OAM_END           0x70003FF

#define MEM_GAMEPAK_ROM_START 0x8000000
#define MEM_GAMEPAK_ROM_END   0xE00FFFF 

// sizes of internal regions
#define MEM_BIOS_SIZE        0x4000
#define MEM_BOARD_WRAM_SIZE  0x40000
#define MEM_CHIP_WRAM_SIZE   0x8000
#define MEM_IO_REG_SIZE      0x400
#define MEM_PALETTE_RAM_SIZE 0x400
#define MEM_VRAM_SIZE        0x18000
#define MEM_OAM_SIZE         0x400

#endif // MEM_COMMON_H