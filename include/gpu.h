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

#include "memory.h"
#include "common.h"

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

        u32 scanline_buffer[SCREEN_WIDTH];
        u32 screen_buffer[SCREEN_HEIGHT * SCREEN_WIDTH];

        // oam data structure
        struct obj_attr
        {
            // coordinates
            u8  y;
            u16 x;

            u8 obj_mode;     // 0 - normal render, 1 - affine, 2 - hidden, 3 - double-wide affine
            u8 gfx_mode;     // 0 - normal, 1 - semi-transparent, 2 - obj window, 3 - illegal
            u8 color_mode;   // 256 color if on, 16 color if off
            bool mosaic;

            u8 affine_index; // P matrix index (0 - 31)
            bool h_flip;
            bool v_flip;

            u16 tileno;      // base tile index of sprite
            u8 priority;
            u8 palbank;      // use in 16 color mode

            u8 size;
            u8 shape;
            
            // width, height of sprite in tiles
            u8 width;
            u8 height;

        } objs[NUM_OBJS]; // can support 128 objects

        // window boundaries
        u16 win0rr, win0ll, win1rr, win1ll; 
        u8  win0tt, win0bb, win1tt, win1bb;

        // video mode draws
        void draw_mode0();
        void draw_mode1();
        void draw_mode2();
        void draw_mode3();
        void draw_mode4();
        void draw_mode5();

        void draw_reg_background(int);
        void draw_affine_background(int);

        // misc
        void render_scanline();
        void draw_sprites();
        void draw_regular_sprite(obj_attr);
        void draw_affine_sprite(obj_attr);
        void update_attr();
};

#endif // GPU_H