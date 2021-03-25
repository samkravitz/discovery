/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 *
 * FILE: PPU.cpp
 * DATE: January 6th, 2021
 * DESCRIPTION: Implementation of PPU class
 */

#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstring>

#include "PPU.h"
#include "util.h"

// transparent pixel color
#define TRANSPARENT 0x8000

PPU::PPU(Memory *mem, LcdStat *stat) : mem(mem), stat(stat)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        LOG(LogLevel::Error, "Could not initialize PPU");
        exit(2);
    }

    window = SDL_CreateWindow("discovery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, 0);

    if (window == NULL)
    {
        LOG(LogLevel::Error, "Could not create window");
        exit(2);
    }

    // discovery icon logo
    SDL_Surface *logo = SDL_LoadBMP("assets/discovery.bmp");

    if (logo == NULL)
    {
        LOG(LogLevel::Error, "Could not load discovery logo!\n");
        exit(2);
    }

    SDL_SetWindowIcon(window, logo);

    final_screen = SDL_GetWindowSurface(window);
    original_screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

    scale_rect.w = SCREEN_WIDTH  * 2;
    scale_rect.h = SCREEN_HEIGHT * 2;
    scale_rect.x = 0;
    scale_rect.y = 0;

    oam_update = std::make_unique<std::stack<int>>();

    Reset();
}

PPU::~PPU()
{
    LOG("PPU: Shutdown\n");
    SDL_Quit();
}

void PPU::Reset()
{
    cycles         = 0;
    scanline       = 0;
    frame          = 0;
    fps            = 0;
    backdrop_color = 0;
    old_time       = clock();
    objminx        = 0;
    objminy        = 0;
    objmaxx        = 240;
    objmaxy        = 160;
    obj_in_winout  = false;

    std::memset(screen_buffer, 0, sizeof(screen_buffer));

    // zero oam data structure
    for (int i = 0; i < NUM_OBJS; ++i)
    {
        std::memset(&objs[i], 0, sizeof(ObjAttr));
        objs[i].obj_mode = 2; // hidden
    }

    // reset window parameters
    for (int i = 0; i < 3; ++i)
    {
        win[i].left   = 0;
        win[i].right  = 240;
        win[i].top    = 0;
        win[i].bottom = 160;
    }
}

// 1 clock cycle of the PPU
void PPU::tick()
{
    cycles++;

    // start HBlank
    if (cycles == HDRAW)
    {
        if (scanline < SCREEN_HEIGHT)
            renderScanline();

        stat->dispstat.in_hBlank = true;

        // fire HBlank interrupt if necessary
        if (stat->dispstat.hbi)
        {
            mem->memory[REG_IF] |= IRQ_HBLANK;
            //LOG(LogLevel::Debug, "HBlank interrupt\n");
        }


        // check for DMA HBlank requests
        // TODO - Don't fire DMA Hblank in VBlank
        if (!stat->dispstat.in_vBlank)
        {
            for (int i = 0; i < 4; ++i)
            {
                if (mem->dma[i].enable && mem->dma[i].mode == 2) // start at HBLANK
                {
                    mem->_dma(i);
                    //LOG(LogLevel::Debug, "DMA {} HBLANK\n", i);
                }
            }
        }

        // start VBlank
        if (scanline == VDRAW)
        {
            render();
            stat->dispstat.in_vBlank = true;

            // fire Vblank interrupt if necessary
            if (stat->dispstat.vbi)
            {
                //LOG(LogLevel::Debug, "VBlank interrupt\n");
                mem->memory[REG_IF] |= IRQ_VBLANK;
            }

            // check for DMA VBLANK requests
            for (int i = 0; i < 4; ++i)
            {
                if (mem->dma[i].enable && mem->dma[i].mode == 1) // start at VBLANK
                {
                    mem->_dma(i);
                    LOG(LogLevel::Debug, "DMA {} VBLANK\n", i);
                }
            }

            // calculate fps
            if (++frame == 60)
            {
                frame = 0;

                double duration;
                clock_t new_time = std::clock();
                duration = (new_time - old_time) / (double) CLOCKS_PER_SEC;
                old_time = new_time;

                std::stringstream stream;
                stream << std::fixed << std::setprecision(1) << (60 / duration);
                std::string title("");
                title += "discovery - ";
                title += stream.str();
                title += " fps";
                SDL_SetWindowTitle(window, title.c_str());
            }
        }
    }

    // completed HBlank
    else if (cycles == HDRAW + HBLANK)
    {
        // completed full refresh
        if (scanline == VDRAW + VBLANK)
        {
            stat->dispstat.in_vBlank = false;
            scanline = 0;
            stat->scanline = 0;
        }

        else
        {
            scanline++;
            stat->scanline++;
        }

        // scanline has reached trigger value
        if (scanline == stat->dispstat.vct)
        {
            // set trigger status
            stat->dispstat.vcs = 1;

            // scanline interrupt is triggered if requested
            if (stat->dispstat.vci)
            {
                mem->memory[REG_IF] |= IRQ_VCOUNT;
                //std::cout << "Scanline interrupt\n";
            }

        }

        // scanline is not equal to trigger value, reset this bit
        else
        {
            stat->dispstat.vcs = 0;
        }

        cycles = 0;
        stat->dispstat.in_hBlank = false;
    }
}

void PPU::render()
{
    //std::cout << "Executing graphics mode: " << (int) (stat->dispcnt.mode) << "\n";

    // copy pixel buffer over to surface pixels
    if (SDL_MUSTLOCK(final_screen))
        SDL_LockSurface(final_screen);

    u32 *screen_pixels = (u32 *) original_screen->pixels;

    std::memcpy(screen_pixels, screen_buffer, sizeof(screen_buffer));

    if (SDL_MUSTLOCK(final_screen))
        SDL_UnlockSurface(final_screen);

    // scale screen buffer
    SDL_BlitScaled(original_screen, NULL, final_screen, &scale_rect);

    // draw final_screen pixels on screen
    SDL_UpdateWindowSurface(window);

    // reset bgcnt's window parameters
    for (int i = 0; i < 4; ++i)
    {
        stat->bgcnt[i].minx = 0;
        stat->bgcnt[i].miny = 0;
        stat->bgcnt[i].maxx = 240;
        stat->bgcnt[i].maxy = 160;
        stat->bgcnt[i].in_winout = false;
        stat->bgcnt[i].in_winin  = false;
    }

    // reset window parameters
    for (int i = 0; i < 3; ++i)
    {
        win[i].left   = 0;
        win[i].right  = 240;
        win[i].top    = 0;
        win[i].bottom = 160;
    }

    // reset obj layer window parameters
    objminx = 0;
    objminy = 0;
    objmaxx = 240;
    objmaxy = 160;
    obj_in_winout = false;
}

void PPU::renderScanline()
{
    // index 0 in BG palette
    backdrop_color = util::u16ToU32Color(mem->read16Unsafe(MEM_PALETTE_RAM_START));

    // "zero" scanline buffer with backdrop color
    for (int i = 0; i < SCREEN_WIDTH; ++i)
        scanline_buffer[i] = backdrop_color;

    // init windows if enabled
    if (stat->dispcnt.win_enabled != 0)
        composeWindow();

    // render bg
    switch (stat->dispcnt.mode)
    {
        case 0: // reg bg 0-3
            // for (int i = 3; i >= 0; --i) // bg0 - bg3
            // {
            //     if (stat->bgcnt[i].enabled)
            //         RenderScanlineText(i);
            // }
            for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
            {
                for (int i = 3; i >= 0; --i) // bg0 - bg3
                {
                    if (stat->bgcnt[i].enabled && stat->bgcnt[i].priority == priority)
                    {
                        renderScanlineText(i);
                    }
                }
            }

            break;
        case 1: // reg bg 0-1, aff bg 2
            // if (stat->bgcnt[2].enabled) RenderScanlineAffine(2);
            // if (stat->bgcnt[1].enabled) RenderScanlineText(1);
            // if (stat->bgcnt[0].enabled) RenderScanlineText(0);
            for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
            {
                for (int i = 2; i >= 0; --i) // bg0 - bg2
                {
                    if (stat->bgcnt[i].enabled && stat->bgcnt[i].priority == priority)
                    {
                        switch (i)
                        {
                            case 0:
                            case 1:
                                renderScanlineText(i);
                                break;
                            case 2:
                                renderScanlineAffine(i);
                                break;
                            default: // should never happen
                                std::cerr << "Error: trying to draw invalid background in mode 1: " << i << "\n";
                        }
                    }
                }
            }

            break;
        case 2: // aff bg 2-3
            // if (stat->bgcnt[3].enabled) RenderScanlineAffine(3);
            // if (stat->bgcnt[2].enabled) RenderScanlineAffine(2);
            for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
            {
                for (int i = 3; i >= 2; --i) // bg3 - bg2
                {
                    if (stat->bgcnt[i].enabled && stat->bgcnt[i].priority == priority)
                    {
                       renderScanlineAffine(i);
                    }
                }
            }

            break;
        case 3:
        case 4:
        case 5:
            renderScanlineBitmap(stat->dispcnt.mode);
            break;
    }

    // render sprites
    if (stat->dispcnt.obj_enabled)
    {
        // update objs list
        updateAttr();
        renderObj();
        //std::memcpy(&screen_buffer[scanline * SCREEN_WIDTH], obj_scanline_buffer, sizeof(obj_scanline_buffer));
    }

    std::memcpy(&screen_buffer[scanline], scanline_buffer, sizeof(scanline_buffer));
}

void PPU::renderScanlineText(int bg)
{
    auto const &bgcnt = stat->bgcnt[bg];

    // scanline outside of window
    if (scanline < bgcnt.miny || scanline > bgcnt.maxy)
       return;

    int pitch; // pitch of screenblocks

    //LOG("({}, {}) ({}, {}) {}\n", bgcnt.minx, bgcnt.miny, bgcnt.maxx, bgcnt.maxy, scanline);

    // width, height of map in pixels
    int width, height;
    switch (bgcnt.size)
    {
        case 0: width = 256; height = 256; pitch = 0; break;
        case 1: width = 512; height = 256; pitch = 0; break;
        case 2: width = 256; height = 512; pitch = 1; break;
        case 3: width = 512; height = 512; pitch = 2; break;
    }

    // map position
    int map_x, map_y = (scanline + bgcnt.voff) % height;

    // tile coordinates (in map)
    int tile_x, tile_y = map_y / 8; // 8 px per tile

    int screenblock, screenentry, se_index;
    int tile_id, hflip, vflip, palbank; // screenentry properties
    int pixel;
    u32 sb_addr, tile_addr;

    // used for vflip / hflip
    int grid_x, grid_y;

    // conver only where bg is inside its window
    for (int x = bgcnt.minx; x < bgcnt.maxx; ++x)
    {
        // layer is in winout & not winin
        if (bgcnt.in_winout && !bgcnt.in_winin && !isInWinOut(x, scanline))
          continue;

        map_x = (x + bgcnt.hoff) % width;
        tile_x = map_x / 8; // 8 px per tile

        screenblock = bgcnt.sbb + ((tile_y / 32) * pitch + (tile_x / 32));
        sb_addr = MEM_VRAM_START + SCREENBLOCK_LEN * (bgcnt.sbb);
        se_index = screenblock * 1024 + (tile_y % 32) * 32 + (tile_x % 32);

        screenentry = mem->read16(MEM_VRAM_START + 2 * se_index);
        tile_id = screenentry >>  0 & 0x3FF;
        hflip  = screenentry >> 10 & 0x1;
        vflip  = screenentry >> 11 & 0x1;

        grid_x = map_x % 8;
        grid_y = map_y % 8;

        if (hflip)
            grid_x = 7 - grid_x;

        if (vflip)
            grid_y = 7 - grid_y;

        if (bgcnt.color_mode == 0) // 4BPP
        {
            palbank = screenentry >> 12 & 0xF;

            tile_addr = (MEM_VRAM_START + bgcnt.cbb * CHARBLOCK_LEN) + 0x20 * tile_id;
            pixel = getBGPixel4BPP(tile_addr, palbank, grid_x, grid_y);
        }

        else // 8BPP
        {
            tile_addr = (MEM_VRAM_START + bgcnt.cbb * CHARBLOCK_LEN) + 0x40 * tile_id;
            pixel = getBGPixel8BPP(tile_addr, grid_x, grid_y);
        }


        if (pixel != TRANSPARENT)
            scanline_buffer[x] = util::u16ToU32Color(pixel);
    }

}

// render the current scanline for affine bg modes
void PPU::renderScanlineAffine(int bg)
{
    auto const &bgcnt = stat->bgcnt[bg];

    // scanline outside of window
    if (scanline < bgcnt.miny || scanline > bgcnt.maxy)
        return;

    // displacement vector
    // width, height of map in pixels
    int width, height;
    switch (bgcnt.size)
    {
        case 0b00: width = height =  128; break;
        case 0b01: width = height =  256; break;
        case 0b10: width = height =  512; break;
        case 0b11: width = height = 1024; break;
    }

    int dx_raw = bgcnt.dx;
    int dy_raw = bgcnt.dy;
    float dx, dy; // displacement vector
    float pa, pb, pc, pd; // P matrix
    switch (bg)
    {
        case 2:
            pa = (s16) mem->read32Unsafe(REG_BG2PA) / 256.0;
            pb = (s16) mem->read32Unsafe(REG_BG2PB) / 256.0;
            pc = (s16) mem->read32Unsafe(REG_BG2PC) / 256.0;
            pd = (s16) mem->read32Unsafe(REG_BG2PD) / 256.0;
            break;

        case 3:
            pa = (s16) mem->read32Unsafe(REG_BG3PA) / 256.0;
            pb = (s16) mem->read32Unsafe(REG_BG3PB) / 256.0;
            pc = (s16) mem->read32Unsafe(REG_BG3PC) / 256.0;
            pd = (s16) mem->read32Unsafe(REG_BG3PD) / 256.0;
            break;
    }

    dx = (float) (dx_raw >> 8) + ((dx_raw & 0xFF) / 256.0f);
    dy = (float) (dy_raw >> 8) + ((dx_raw & 0xFF) / 256.0f);

    //dx += pb * scanline;
    //dy += pd * scanline;

    float x0 = dx, y0 = dy;
    int   x1,      y1 = scanline + dy;
    int   px,      py;
    
    // map position
    int map_x, map_y;

    // tile coordinates (in map)
    int tile_x, tile_y;

    int se_index;
    int pixel;
    u32 tile_addr;

    for (int x = bgcnt.minx; x < bgcnt.miny; ++x)
    {
        // layer is in winout & not winin
        if (bgcnt.in_winout && !bgcnt.in_winin && !isInWinOut(x, scanline))
          continue;

        x1 = x + dx;

        // affine transform
        px = pa * (x1 - x0) + pb * (y1 - y0) + x0;
        py = pc * (x1 - x0) + pd * (y1 - y0) + y0;

        // wrap
        if (bgcnt.affine_wrap == 1)
        {
            if (px < 0)
                px = width  - std::abs(px);
            if (py < 0)
                py = height - std::abs(py);

            px %=  width;
            py %=  height;
        }

        // no wrap
        else
        {
            // transformmed coordinate is out of bounds
            if (px >= width || py >= height) continue;
            if (px < 0      || py < 0)       continue;
        }

        map_x = px;
        map_y = py;
        tile_x = map_x / 8; // 8 px per tile
        tile_y = map_y / 8;

        se_index = mem->read8((MEM_VRAM_START + bgcnt.sbb * SCREENBLOCK_LEN) + tile_y * (width / 8) + tile_x);
        tile_addr = (MEM_VRAM_START + bgcnt.cbb * CHARBLOCK_LEN) + (se_index * 0x40);

        // 8BPP only
        pixel = getBGPixel8BPP(tile_addr, map_x % 8, map_y % 8);

        if (pixel != TRANSPARENT)
            scanline_buffer[x] = util::u16ToU32Color(pixel);
    }
}

// render the current scanline for bitmap modes
void PPU::renderScanlineBitmap(int mode)
{
    u8 palette_index;
    u16 pixel;
    u32 pal_ptr;

    switch (mode)
    {
        case 3:
            pal_ptr = MEM_VRAM_START + (scanline * SCREEN_WIDTH * sizeof(u16));

            for (int i = 0; i < SCREEN_WIDTH; ++i)
            {
                pixel = mem->read16Unsafe(pal_ptr); pal_ptr += 2;
                scanline_buffer[i] = util::u16ToU32Color(pixel);
            }
            break;

        case 4:
            pal_ptr = MEM_VRAM_START + (scanline * SCREEN_WIDTH);

            // page 2 starts at 0x600A000
            if (stat->dispcnt.ps)
                pal_ptr += 0xA000;

            for (int i = 0; i < SCREEN_WIDTH; ++i)
            {
                palette_index = mem->read8Unsafe(pal_ptr++);
                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                pixel = mem->read16Unsafe(MEM_PALETTE_RAM_START + (palette_index * sizeof(u16)));
                scanline_buffer[i] = util::u16ToU32Color(pixel);
            }

            break;

        case 5:
            if (scanline >= 128) return; // mode 5 has 160 x 128 resolution

            pal_ptr = MEM_VRAM_START + (scanline * 160);

            // page 2 starts at 0x600A000
            if (stat->dispcnt.ps)
                pal_ptr += 0xA000;

            for (int i = 0; i < 160; ++i)
            {
                palette_index = mem->read8Unsafe(pal_ptr++);
                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                pixel = mem->read16Unsafe(MEM_PALETTE_RAM_START + (palette_index * sizeof(u16)));
                scanline_buffer[i] = util::u16ToU32Color(pixel);
            }

            break;
    }
}


void PPU::renderObj()
{
    u16 pixel;

    // loop through all objs
    while (!oam_update->empty())
    {
        auto *attr = &objs[oam_update->top()];
        oam_update->pop();

        // skip hidden object
        if (attr->obj_mode == 2)
            continue;

        // obj exists outside current scanline
        if (scanline < attr->qy0 - attr->hheight || scanline >= attr->qy0 + attr->hheight)
             continue;

        // obj exists outside current object layer
        if (scanline < objminy || scanline >= objmaxy)
            continue;
        
        int qx0 = attr->qx0;      // center of sprite screen space

        // x, y coordinate of texture after transformation
        int px, py;
        int iy = -attr->hheight + (scanline - attr->y);


        //LOG("{} {} {} {}\n", attr->x, attr->y, attr->hheight, attr->hwidth);
        //LOG("{} {} {} {}\n", attr->x0, attr->y0, attr->hheight, attr->hwidth);

        for (int ix = -attr->hwidth; ix < attr->hwidth; ++ix)
        {
            // objs in winout
            if (obj_in_winout && !isInWinOut(qx0 + ix, scanline))
                continue;

            px = ix + attr->hwidth;
            py = iy + attr->hheight;

            // obj exists outside current object layer
            if (qx0 + ix < objminx || qx0 + ix >= objmaxx)
                continue;

            // transform affine & double wide affine
            if (attr->obj_mode == 1 || attr->obj_mode == 3)
            {
                px = attr->pa * ix + attr->pb * iy + attr->px0;
                py = attr->pc * ix + attr->pd * iy + attr->py0;
            }

            // horizontal / vertical flip
            if (attr->h_flip) px = attr->width  - px - 1;
            if (attr->v_flip) py = attr->height - py - 1;
            
            // transformed coordinate is out of bounds
            if (px >= attr->width || py  >= attr->height) continue;
            if (px       < 0      || py  < 0            ) continue;
            if (qx0 + ix < 0      || qx0 + ix >= 240    ) continue;

            
            int tile_x  = px % 8; // x coordinate of pixel within tile
            int tile_y  = py % 8; // y coordinate of pixel within tile
            int block_x = px / 8; // x coordinate of tile in vram
            int block_y = py / 8; // y coordinate of tile in vram

            int tileno = attr->tileno;
            int pixel;

            if (attr->color_mode == 1) // 8bpp
            {
                if (stat->dispcnt.obj_map_mode == 1) // 1d
                    tileno += block_y * (attr->width / 4);

                else // 2d
                    tileno = (tileno & ~1) + block_y * 32;
                
                tileno += block_x * 2;

                pixel = getObjPixel8BPP(LOWER_SPRITE_BLOCK + tileno * 32, tile_x, tile_y);
            }

            else // 4bpp
            {
                if (stat->dispcnt.obj_map_mode == 1) // 1d
                    tileno += block_y * (attr->width / 8);

                else // 2d
                    tileno += block_y * 32;

                tileno += block_x;

                pixel = getObjPixel4BPP(LOWER_SPRITE_BLOCK + tileno * 32, attr->palbank, tile_x, tile_y);
            }
            
            if (pixel != TRANSPARENT)
                scanline_buffer[qx0 + ix] = util::u16ToU32Color(pixel);
        }
    }
}

void PPU::updateAttr()
{
    u32 oam_ptr = MEM_OAM_START;
    u16 attr0, attr1, attr2;

    // loop through all objs
    for (int i = 0; i < NUM_OBJS; ++i)
    {
        ObjAttr &obj = objs[i];

        attr0 = mem->read16Unsafe(oam_ptr); oam_ptr += 2;
        attr1 = mem->read16Unsafe(oam_ptr); oam_ptr += 2;
        attr2 = mem->read16Unsafe(oam_ptr); oam_ptr += 4;

        obj.y            = attr0 >>  0 & 0xFF;
        obj.obj_mode     = attr0 >>  8 & 0x3;
        obj.gfx_mode     = attr0 >> 10 & 0x3;
        obj.mosaic       = attr0 >> 12 & 0x1;
        obj.color_mode   = attr0 >> 13 & 0x1;
        obj.shape        = attr0 >> 14 & 0x3;

        obj.x            = attr1 >>  0 & 0x1FF;
        obj.affine_index = attr1 >>  9 & 0x1F;
        obj.h_flip       = attr1 >> 12 & 0x1;
        obj.v_flip       = attr1 >> 13 & 0x1;
        obj.size         = attr1 >> 14 & 0x3;

        obj.tileno       = attr2 >>  0 & 0x3FF;
        obj.priority     = attr2 >> 10 & 0x3;
        obj.palbank      = attr2 >> 12 & 0xF;

        if (obj.x >= SCREEN_WIDTH)  obj.x -= 512;
        if (obj.y >= SCREEN_HEIGHT) obj.y -= 256;

        // get actual dimensions of sprite
        switch (obj.shape)
        {
            case 0:
                switch (obj.size)
                {
                    case 0: obj.width =  8; obj.height =  8; break;
                    case 1: obj.width = 16; obj.height = 16; break;
                    case 2: obj.width = 32; obj.height = 32; break;
                    case 3: obj.width = 64; obj.height = 64; break;
                }
                break;

            case 1:
                switch (obj.size)
                {
                    case 0: obj.width = 16; obj.height =  8; break;
                    case 1: obj.width = 32; obj.height =  8; break;
                    case 2: obj.width = 32; obj.height = 16; break;
                    case 3: obj.width = 64; obj.height = 32; break;
                }
                break;

            case 2:
                switch (obj.size)
                {
                    case 0: obj.width =  8; obj.height = 16; break;
                    case 1: obj.width =  8; obj.height = 32; break;
                    case 2: obj.width = 16; obj.height = 32; break;
                    case 3: obj.width = 32; obj.height = 64; break;
                }
                break;

            default: // prohibited
                obj.width = 0; obj.height = 0;
        }

        // hwidth, hheight
        obj.hwidth  = obj.width  / 2;
        obj.hheight = obj.height / 2;

        obj.qx0 = obj.x + obj.hwidth;
        obj.qy0 = obj.y + obj.hheight;
        obj.px0 = obj.hwidth;
        obj.py0 = obj.hheight;

        // get affine matrix if necessary
        if (obj.obj_mode == 1 || obj.obj_mode == 3) // affine
        {
            u32 matrix_ptr = MEM_OAM_START + obj.affine_index * 32; // each affine entry is 32 bytes across

            // transform P matrix from 8.8f to float
            // P = [pa pb]
            //     [pc pd]
            obj.pa = (s16) mem->read16(matrix_ptr +  0x6) / 256.0;
            obj.pb = (s16) mem->read16(matrix_ptr +  0xE) / 256.0;
            obj.pc = (s16) mem->read16(matrix_ptr + 0x16) / 256.0;
            obj.pd = (s16) mem->read16(matrix_ptr + 0x1E) / 256.0;

            // double wide affine
            if (obj.obj_mode == 3)
            {
                obj.qx0 += obj.hwidth;
                obj.qy0 += obj.hheight;

                obj.hwidth  *= 2;
                obj.hheight *= 2;
            }

            // make sure flips are set to zero
            obj.v_flip = 0;
            obj.h_flip = 0;
        }

        // add index to stack to be displayed
        if (obj.obj_mode != 2)
            oam_update->push(i);
    }
}

void PPU::composeWindow()
{
    // win1 enabled
    if (stat->dispcnt.win_enabled & 2)
    {
        u16 win1v  = mem->read16Unsafe(REG_WIN1V);
        u16 win1h  = mem->read16Unsafe(REG_WIN1H);
        
        auto &window = win[1];
        window.left   = win1h >> 8;
        window.right  = win1h & 0xFF;
        window.top    = win1v >> 8;
        window.bottom = win1v & 0xFF;

        // illegal values
        if (window.right > 240 || window.left > window.right)
            window.right = 240;

        if (window.bottom > 160 || window.top > window.bottom)
            window.bottom = 160;

        // window content
        u8 win1content = mem->read16Unsafe(REG_WININ) >> 8;

        // check if bgs are in window content
        int mask;
        for (int i = 0; i < 4; ++i)
        {
            mask = 1 << i;
            if (win1content & mask)
            {
                stat->bgcnt[i].minx = window.left;
                stat->bgcnt[i].maxx = window.right;
                stat->bgcnt[i].miny = window.top;
                stat->bgcnt[i].maxy = window.bottom;
                stat->bgcnt[i].in_winin = true;
            }
        }

        // objs in win1
        if (win1content & 0x10)
        {
            objminx        = window.left;
            objmaxx        = window.right;
            objminy        = window.top;
            objmaxy        = window.bottom;
            obj_in_winout  = false;
        }
    }

    // win0 enabled
    if (stat->dispcnt.win_enabled & 1)
    {
        u16 win0v  = mem->read16Unsafe(REG_WIN0V);
        u16 win0h  = mem->read16Unsafe(REG_WIN0H);

        auto &window = win[0];
        window.left   = win0h >> 8;
        window.right  = win0h & 0xFF;
        window.top    = win0v >> 8;
        window.bottom = win0v & 0xFF;

        // illegal values
        if (window.right > 240 || window.left > window.right)
            window.right = 240;

        if (window.bottom > 160 || window.top > window.bottom)
            window.bottom = 160;

        // window content
        u8 win0content = mem->read16Unsafe(REG_WININ) & 0xFF;

        // check if bgs are in window content
        int mask;
        for (int i = 0; i < 4; ++i)
        {
            mask = 1 << i;
            if (win0content & mask)
            {
                stat->bgcnt[i].minx = window.left;
                stat->bgcnt[i].maxx = window.right;
                stat->bgcnt[i].miny = window.top;
                stat->bgcnt[i].maxy = window.bottom;
                stat->bgcnt[i].in_winin = true;
            }
        }

        // objs in win0
        if (win0content & 0x10)
        {
            objminx        = window.left;
            objmaxx        = window.right;
            objminy        = window.top;
            objmaxy        = window.bottom;
            obj_in_winout  = false;
        }
    }

    // winout
    u8 winoutcontent = mem->read16Unsafe(REG_WINOUT) & 0xFF;

    // check if bgs are in winout content
    int mask;
    for (int i = 0; i < 4; ++i)
    {
        mask = 1 << i;
        if (winoutcontent & mask)
        {
            stat->bgcnt[i].minx = 0;
            stat->bgcnt[i].miny = 0;
            stat->bgcnt[i].maxx = 240;
            stat->bgcnt[i].maxy = 160;
            stat->bgcnt[i].in_winout = true;
        }
    }

    // objs in winout
    if (winoutcontent & 0x10)
        obj_in_winout = true;

    // obj window enabled
    if (stat->dispcnt.win_enabled & 4)
        LOG(LogLevel::Warning, "Object window is enabled\n");
}

inline u16 PPU::getObjPixel4BPP(u32 addr, int palbank, int x, int y)
{
    addr += (y * 4) + (x / 2);

    u16 palette_index = mem->read8(addr);

    // use top nybble for odd x, even otherwise
    if (x & 1) { palette_index >>= 4; }

    palette_index &= 0xF;

    if (palette_index == 0)
        return TRANSPARENT;

    return mem->read16(SPRITE_PALETTE + palette_index * sizeof(u16) + (palbank * PALBANK_LEN));
}

inline u16 PPU::getObjPixel8BPP(u32 addr, int x, int y)
{
    addr += (y * 8) + x;

    u16 palette_index = mem->read8(addr);

   if (palette_index == 0)
        return TRANSPARENT;

    return mem->read16(SPRITE_PALETTE + palette_index * sizeof(u16));
}

inline u16 PPU::getBGPixel4BPP(u32 addr, int palbank, int x, int y)
{
    addr += (y * 4) + (x / 2);

    u16 palette_index = mem->read8(addr);

    // use top nybble for odd x, even otherwise
    if (x & 1) { palette_index >>= 4; }

    palette_index &= 0xF;

    if (palette_index == 0)
        return TRANSPARENT;

    return mem->read16(BG_PALETTE + palette_index * sizeof(u16) + (palbank * PALBANK_LEN));
}

inline u16 PPU::getBGPixel8BPP(u32 addr, int x, int y)
{
    addr += (y * 8) + x;

    u16 palette_index = mem->read8(addr);

    if (palette_index == 0)
       return TRANSPARENT;

    return mem->read16(BG_PALETTE + palette_index * sizeof(u16));
}

// returns true if (x, y) is currently in winOut, true otherwise
bool PPU::isInWinOut(int x, int y)
{
    if ((stat->dispcnt.win_enabled & 1)         &&
        x >= win[0].left && x <= win[0].right   &&
        y >= win[0].top  && y <= win[0].bottom) return false;

    if ((stat->dispcnt.win_enabled & 2)         &&
        x >= win[1].left && x <= win[1].right   &&
        y >= win[1].top  && y <= win[1].bottom) return false;
    
    return true;
}

void PPU::printPalette()
{
    for (int i = 0; i < 256; i++)
    {
        LOG("{}: {:x}\n", i, mem->read16Unsafe(MEM_PALETTE_RAM_START + 2 * i));
    }
}