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
#include "common/memory.h"
#include "common/common.h"
#include "common/gpu.h"
#include "memory.h"
#include "lcd_stat.h"
#include "obj_attr.h"

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 160

#define HDRAW_CYCLES     960
#define SCANLINE_CYCLES  1232
#define NUM_SCANLINES    228
#define REFRESH_CYCLES   280896

#define NUM_OBJS 128 // number of sprites that can be rendered

class GPU {
    public:
        GPU();
        ~GPU();

        Memory *mem;
        lcd_stat *stat;

        u32 lcd_clock;
        u8 current_scanline;

        void clock_gpu();
        
        void reset();
        void draw();

        clock_t old_clock;

    private:
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *texture;

        // video mode draws
        void draw_mode0();
        void draw_mode3();
        void draw_mode4();

        // misc
        obj_attr get_attr(int);
        void draw_sprites();
        void draw_sprite(obj_attr, u32 *);
        void draw_tile(int, int, u32 *);
};

#endif // GPU_H