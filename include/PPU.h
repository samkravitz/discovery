/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: PPU.h
 * DATE: July 13, 2020
 * DESCRIPTION: class definition for graphics processing unit (gpu)
 */
#pragma once

#include <SDL2/SDL.h>
#include <iostream>
#include <ctime>
#include <functional>
#include <stack>
#include <array>
#include <vector>

#include "Memory.h"
#include "common.h"
#include "mmio.h"

constexpr int SCREEN_WIDTH        = 240;
constexpr int SCREEN_HEIGHT       = 160;
constexpr int MAX_X               = 512;
constexpr int MAX_Y               = 256;

constexpr int HDRAW               = 960; // # of cycles in HDraw
constexpr int HBLANK              = 272; // # of cycles in HBlank
constexpr int VDRAW               = 160; // # of scanlines in VDraw
constexpr int VBLANK              = 68;  // # of scanlines in VBlank

constexpr int CHARBLOCK_LEN       = 0x4000;
constexpr int SCREENBLOCK_LEN     = 0x800;

constexpr int PALBANK_LEN         = 32; // length of each of palette RAM's 16 banks in 4bpp mode (s-tiles)

constexpr int NUM_OBJS            = 128; // number of sprites that can be rendered
constexpr int NUM_BG              = 4;

constexpr u32 LOWER_SPRITE_BLOCK  = 0x6010000;
constexpr u32 HIGHER_SPRITE_BLOCK = 0x6014000;

constexpr u32 BG_PALETTE          = 0x5000000;
constexpr u32 SPRITE_PALETTE      = 0x5000200;


class PPU
{
public:
    PPU(Memory *, LcdStat *);
    ~PPU();

    Memory *mem;
    LcdStat *stat;

    u32 cycles;
    u8 scanline;

    void tick();
    void reset();

private:
    SDL_Window  *window;
    SDL_Surface *final_screen;
    SDL_Surface *original_screen;
    SDL_Rect    scale_rect;

    // internal buffers linked from memory
    u8 *palram;
    u8 *vram;
    u8 *oam;

    u8 frame; // counts 0 - 60
    u8 fps;
    clock_t old_time;

    u32 scanline_buffer[SCREEN_WIDTH];

    struct ObjPixel
    {
        u16 color;
        int priority;        
    } obj_scanline_buffer[SCREEN_WIDTH];

    int objwin_scanline_buffer[SCREEN_WIDTH];

    std::array<u16, SCREEN_WIDTH> bg_buffer[NUM_BG];
    std::vector<int> bg_list; // list of currently enabled bgs

    u32 screen_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];

    // oam data structure
    struct ObjAttr
    {
        int idx;

        // coordinate of top left of sprite
        int x, y;

        // origin in screen space (q) and texture space (p)
        int qx0, qy0;
        int px0, py0;

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
    void renderScanline();
    void renderScanlineText(int);
    void renderScanlineAffine(int);
    void renderScanlineBitmap(int);
    void renderScanlineObj();

    // misc
    inline u16 getObjPixel4BPP(u32, int, int, int);
    inline u16 getObjPixel8BPP(u32, int, int);
    inline u16 getBGPixel4BPP(u32, int, int, int);
    inline u16 getBGPixel8BPP(u32, int, int);
    void updateAttr();
    inline bool isInWindow(int, int, int);

    u32 color_lut[0x10000];
    inline u32 u16ToU32Color(u16);
};