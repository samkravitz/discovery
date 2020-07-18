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

class GPU {
    public:
        GPU();
        ~GPU();

        void init();
        void draw_pixel(int, int);

    private:
        SDL_Window *window;
        SDL_Renderer *renderer;
};

#endif // GPU_H