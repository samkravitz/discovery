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
#include "../memory/common.h"
#include "../common.h"

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 160

class GPU {
    public:
        GPU();
        ~GPU();

        void reset();
        void draw(u32, u8 *);
        void draw_pixel(int, int);

    private:
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Surface *surface;

        // video mode draws
        void draw_mode3(u8 *);
};

#endif // GPU_H