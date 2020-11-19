/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: gpu.h
 * DATE: July 13, 2020
 * DESCRIPTION: class definition for graphics processing unit (gpu)
 */
#ifndef GPU_H
#define GPU_H

#include <SDL2/SDL.h>
#include <iostream>
#include <ctime>

#include "gpu.h"
#include "memory.h"
#include "common.h"
#include "obj_attr.h"

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 160
#define MAX_X         512
#define MAX_Y         256

#define HDRAW         960 // # of cycles in HDraw
#define HBLANK        272 // # of cycles in HBlank
#define VDRAW         160 // # of scanlines in VDraw
#define VBLANK        68  // # of scanlines in VBlank

#define NUM_OBJS 128 // number of sprites that can be rendered

const u32 LOWER_SPRITE_BLOCK = 0x6010000;
const u32 HIGHER_SPRITE_BLOCK = 0x6014000;

const u32 SPRITE_PALETTE = 0x5000200;

class GPU
{
    public:
        GPU();
        ~GPU();

        Memory *mem;
        lcd_stat *stat;

        u32 cycles;
        u8 scanline;

        void cycle();
        
        void reset();
        void draw();

    private:
        SDL_Window  *window;
        SDL_Surface *final_screen;
        SDL_Surface *original_screen;
        SDL_Rect     scale_rect;

        u8 frame; // counts 0 - 60
        u8 fps;
        clock_t old_time;

        u32 screen_buffer[MAX_X][MAX_Y];

        // oam data structure
        obj_attr objs[NUM_OBJS]; // can support 128 normal objects

        // video mode draws
        void draw_mode0();
        void draw_mode1();
        void draw_mode3();
        void draw_mode4();
        void draw_mode5();

        void draw_reg_background(int);
        void draw_affine_background(int);

        // misc
        obj_attr get_attr(int);
        void draw_sprites();
        void draw_regular_sprite(obj_attr);
        void draw_affine_sprite(obj_attr);
        void update_attr();
};

#endif // GPU_H