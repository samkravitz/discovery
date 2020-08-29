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

#include "common/memory.h"
#include "common/common.h"
#include "memory.h"
#include "lcd_stat.h"

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 160

#define NUM_SCANLINES 160

class GPU {
    public:
        GPU();
        ~GPU();

        Memory *mem;
        lcd_stat *stat;

        u32 lcd_clock;
        u8 current_scanline;

        void clock();
        
        void reset();
        void draw();

    private:
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *texture;

        // video mode draws
        void draw_mode3();
        void draw_mode4();
};

#endif // GPU_H