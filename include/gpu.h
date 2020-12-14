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

#define NUM_OBJS      128 // number of sprites that can be rendered

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

    private:
        SDL_Window  *window;
        SDL_Surface *final_screen;
        SDL_Surface *original_screen;
        SDL_Rect     scale_rect;

        u8 frame; // counts 0 - 60
        u8 fps;
        clock_t old_time;

        u32 scanline_buffer[SCREEN_WIDTH];
        u32 obj_scanline_buffer[SCREEN_WIDTH];

        u32 screen_buffer[SCREEN_HEIGHT * SCREEN_WIDTH];

        // oam data structure
        struct obj_attr
        {
            // coordinate of top left (x0, y0) & origin 
            int x,  y;
            int x0, y0;

            int obj_mode;     // 0 - normal render, 1 - affine, 2 - hidden, 3 - double-wide affine
            int gfx_mode;     // 0 - normal, 1 - semi-transparent, 2 - obj window, 3 - illegal
            int color_mode;   // 256 color if on, 16 color if off
            bool mosaic;

            int affine_index; // P matrix index (0 - 31)
            bool h_flip;
            bool v_flip;

            int tileno;       // base tile index of sprite
            int priority;
            int palbank;      // use in 16 color mode

            int size;
            int shape;
            
            // width, height of sprite in pixels (& half width, height)
            int  width,  height;
            int hwidth, hheight;

            // affine matrix params
            float pa;
            float pb;
            float pc;
            float pd;

        } objs[NUM_OBJS]; // can support 128 objects

        // video mode renders
        void render();
        void render_scanline();
        void render_text_scanline(int);
        void render_bitmap_scanline(int);
        void render_obj_scanline();

        void draw_reg_background(int);
        void draw_affine_background(int);

        // misc
        u16 get_obj_pixel4BPP(u32, int, int, int);
        u16 get_obj_pixel8BPP(u32, int, int);
        void update_attr();
};

#endif // GPU_H