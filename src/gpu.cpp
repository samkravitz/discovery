#include <ctime>
#include <sstream>
#include <iomanip>

#include "gpu.h"

#define S_TILE_LEN 32
#define D_TILE_LEN 64

#define PX_IN_TILE_ROW 8
#define PX_IN_TILE_COL 8

// length of each of palette RAM's 16 banks
// in 4bpp mode (s-tiles)
#define PALBANK_LEN 32

#define CHARBLOCK_LEN             0x4000
#define SCREENBLOCK_LEN           0x800
#define S_SCREENENTRY_DEPTH       32    // s-tile SE depth
#define D_SCREENENTRY_DEPTH       64    // d-tile SE depth
#define TILES_PER_SCREENBLOCK     32

// transparent pixel color
#define TRANSPARENT 0x01010101

u32 u16_to_u32_color(u16);

GPU::GPU()
{
    mem  = NULL;
    stat = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Could not initialize GPU" << "\n";
        exit(2);
    }

    window = SDL_CreateWindow("discovery", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, 0);

    if (window == NULL)
    {
        std::cerr << "Could not create window" << "\n";
        exit(2);
    }

    // discovery icon logo
    SDL_Surface *logo = SDL_LoadBMP("assets/discovery.bmp");

    if (logo == NULL)
    {
        std::cerr << "Could not load discovery logo!\n";
        exit(2);
    }
    
    SDL_SetWindowIcon(window, logo);

    final_screen = SDL_GetWindowSurface(window);
    original_screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

    scale_rect.w = SCREEN_WIDTH * 2;
    scale_rect.h = SCREEN_HEIGHT * 2;
    scale_rect.x = 0;
    scale_rect.y = 0;

    reset();
}

GPU::~GPU()
{
    std::cout << "GPU:: Shutdown\n";
    SDL_Quit();
}

void GPU::reset()
{
    cycles    = 0;
    scanline  = 0;
    frame     = 0;
    fps       = 0;
    win0rr    = 0xFFFF;
    win1rr    = 0xFFFF;
    win0bb    = 0xFF;
    win1bb    = 0xFF;
    win0ll    = 0;
    win0ll    = 0;
    win1ll    = 0;
    win1ll    = 0;
    old_time  = clock();
    memset(screen_buffer, 0, sizeof(screen_buffer));

    // zero oam data structure
    for (int i = 0; i < NUM_OBJS; ++i)
    {
        objs[i].obj_mode = 2; // hidden

        objs[i].y            = 0;
        objs[i].gfx_mode     = 0;
        objs[i].mosaic       = 0;
        objs[i].color_mode   = 0;
        objs[i].size         = 0;
        objs[i].x            = 0;
        objs[i].affine_index = 0;
        objs[i].h_flip       = 0;
        objs[i].v_flip       = 0;
        objs[i].shape        = 0;
        objs[i].tileno       = 0;
        objs[i].priority     = 0;
        objs[i].palbank      = 0;
        objs[i].width        = 0;
        objs[i].height       = 0;
    }
}

// 1 clock cycle of the gpu
void GPU::cycle()
{
    cycles++;

    // start HBlank
    if (cycles == HDRAW)
    {
        stat->dispstat.in_hBlank = true;

        // fire HBlank interrupt if necessary
        if (stat->dispstat.hbi)
        {
            mem->memory[REG_IF] |= IRQ_HBLANK;
            //std::cout << "HBlank interrupt\n";
        }
            

        // check for DMA HBlank requests
        // TODO - Don't fire DMA Hblank in VBlank
        for (int i = 0; i < 4; ++i)
        {
            if (mem->dma[i].enable && mem->dma[i].mode == 2) // start at HBLANK
            {
                mem->_dma(i);
                std::cout << "DMA" << i << " HBLANK\n";
            }
        }
        
        // start VBlank
        if (scanline == VDRAW)
        {
            draw();
            stat->dispstat.in_vBlank = true;

            // fire Vblank interrupt if necessary
            if (stat->dispstat.vbi)
            {
                //std::cout << "VBlank interrupt\n";
                mem->memory[REG_IF] |= IRQ_VBLANK;
            }
                
            // check for DMA VBLANK requests
            for (int i = 0; i < 4; ++i)
            {
                if (mem->dma[i].enable && mem->dma[i].mode == 1) // start at VBLANK
                {
                    mem->_dma(i);
                    std::cout << "DMA" << i << " VBLANK\n";
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

void GPU::draw()
{
    //std::cout << "Executing graphics mode: " << (int) (stat->dispcnt.mode) << "\n";
    
    // update window boundaries

    // win0 enabled
    if (stat->dispcnt.win_enabled & 0x1)
    {
        //std::cout << "WIN0 EN\n";
        u16 win0h, win0v;
        win0h = mem->read_u16_unprotected(REG_WIN0H);
        win0v = mem->read_u16_unprotected(REG_WIN0V);

        win0rr = (win0h >> 0) & 0xFF;
        win0ll = (win0h >> 8) & 0xFF;
        win0bb = (win0v >> 0) & 0xFF;
        win0tt = (win0v >> 8) & 0xFF;

        // std::cout << (int) win0ll << "\n";
        // std::cout << (int) win0tt << "\n";
        // std::cout << (int) win0rr << "\n";
        // std::cout << (int) win0bb << "\n";
    }

    // win0 disabled
    else
    {
        //std::cout << "WIN0\n";
        win0rr = 0xFFFF;
        win0ll = 0;
        win0bb = 0xFF;
        win0tt = 0;
    }

    // win1 enabled
    if (stat->dispcnt.win_enabled & 0x2)
    {
        //std::cout << "WIN1 EN\n";
        u16 win1h, win1v;
        win1h = mem->read_u16_unprotected(REG_WIN1H);
        win1v = mem->read_u16_unprotected(REG_WIN1V);

        win1rr = (win1h >> 0) & 0xFF;
        win1ll = (win1h >> 8) & 0xFF;
        win1bb = (win1v >> 0) & 0xFF;
        win1tt = (win1v >> 8) & 0xFF;

        // std::cout << (int) win1ll << "\n";
        // std::cout << (int) win1tt << "\n";
        // std::cout << (int) win1rr << "\n";
        // std::cout << (int) win1bb << "\n";
    }

    // win1 disabled
    else
    {
        //std::cout << "WIN1\n";
        win1rr = 0xFFFF;
        win1ll = 0;
        win1bb = 0xFF;
        win1tt = 0;
    }
    
    // draw background
    switch (stat->dispcnt.mode)
    {
        case 0: draw_mode0(); break;
        case 1: draw_mode1(); break;
        case 2: draw_mode2(); break;
        case 3: draw_mode3(); break;
        case 4: draw_mode4(); break;
        case 5: draw_mode5(); break;
        default: 
            std::cerr << "Error: unknown video mode" << "\n";
            break;
    }

    // win0 enabled & obj is only content of win0
    if ((stat->dispcnt.win_enabled & 0x1) && ((mem->read_u16_unprotected(REG_WININ) & 0x3F) == 0x10))
    {
        for (int y = win0tt; y < win0bb; ++y)
        {
            for (int x = win0ll; x < win0rr; ++x)
            {
                screen_buffer[y][x] = 0;
            }
        }
    }

    // win1 enabled & obj is only content of win1
    if ((stat->dispcnt.win_enabled & 0x2) && ((mem->read_u16_unprotected(REG_WININ) >> 8 & 0x3F) == 0x10))
    {
        for (int y = win1tt; y < win1bb; ++y)
        {
            for (int x = win1ll; x < win1rr; ++x)
            {
                screen_buffer[y][x] = 0;
            }
        }
    }

    // sprites enabled
    if (stat->dispcnt.obj_enabled)
    {
        update_attr();
        draw_sprites();
    }

    // copy pixel buffer over to surface pixels
    if (SDL_MUSTLOCK(final_screen))
        SDL_LockSurface(final_screen);

    u32 *screen_pixels = (u32 *) original_screen->pixels;
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x)
            screen_pixels[y * SCREEN_WIDTH + x] = screen_buffer[y][x];

    if (SDL_MUSTLOCK(final_screen))
        SDL_UnlockSurface(final_screen);

    // scale screen buffer 
    SDL_BlitScaled(original_screen, NULL, final_screen, &scale_rect);
    
    // draw final_screen pixels on screen
    SDL_UpdateWindowSurface(window);
    
    // zero screen buffer for next frame
    if (!stat->dispcnt.fb)
        memset(screen_buffer, 0xFF, sizeof(screen_buffer)); // white
    else
        memset(screen_buffer, 0, sizeof(screen_buffer));   // black
}

// video mode 0 - tile mode
// can draw bg0 - bg3 all regular
void GPU::draw_mode0()
{
    for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
    {
        for (int i = 3; i >= 0; --i) // bg0 - bg3
        {
            if (stat->bg_cnt[i].enabled && stat->bg_cnt[i].priority == priority)
            {
                draw_reg_background(i);
            }
        }
    }
}

// video mode 1 - tile mode
// can draw bg0, bg1 regular and bg 2 affine
void GPU::draw_mode1()
{
    for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
    {
        for (int i = 2; i >= 0; --i) // bg0 - bg2
        {
            if (stat->bg_cnt[i].enabled && stat->bg_cnt[i].priority == priority)
            {
                switch (i)
                {
                    case 0:
                    case 1:
                        draw_reg_background(i);
                        break;
                    case 2:
                        draw_affine_background(i);
                        break;
                    default: // should never happen
                        std::cerr << "Error: trying to draw invalid background in mode 1: " << i << "\n";
                }
            }
        }
    }
}

// video mode 2 - tile mode
// can draw bg2 & bg3 affine
void GPU::draw_mode2()
{
    for (int priority = 3; priority >= 0; --priority) // draw highest priority first, lower priorities drawn on top
    {
        for (int i = 3; i >= 2; --i) // bg3 - bg2
        {
            if (stat->bg_cnt[i].enabled && stat->bg_cnt[i].priority == priority)
            {
                draw_affine_background(i);
            }
        }
    }
}


// video mode 3 - bitmap mode
// mode 3 straight up uses 2 bytes to represent each pixel in aRBG format, no palette used
void GPU::draw_mode3()
{
    u16 current_pixel; // in mode 3 each pixel uses 2 bytes

    int i = 0;
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
    {
        for (int x = 0; x < SCREEN_WIDTH; ++x)
        {
            current_pixel = mem->read_u16_unprotected(MEM_VRAM_START + (i++ * sizeof(u16))); // multiply i * 2 b/c each pixel is 2 bytes
            screen_buffer[y][x] = u16_to_u32_color(current_pixel);
        }
    }
}

// video mode 4 - bitmap mode
void GPU::draw_mode4()
{
    u8 palette_index;                  // in mode 4 each pixel uses 1 byte 
    u16 color;                         // the color located at pallette_ram[palette_index]
    u32 pal_ptr = MEM_VRAM_START;      // address of current palette

    // page 2 starts at 0x600A000
    if (stat->dispcnt.ps)
        pal_ptr += 0xA000;

    for (int y = 0; y < SCREEN_HEIGHT; ++y)
    {
        for (int x = 0; x < SCREEN_WIDTH; ++x)
        {
            palette_index = mem->read_u8_unprotected(pal_ptr++);
            // multiply by sizeof(u16) because each entry in palram is 2 bytes
            color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + (palette_index * sizeof(u16)));
            // add current pixel in argb format to pixel array
            screen_buffer[y][x] = u16_to_u32_color(color);
        }
    }
}

// video mode 5 - bitmap mode
void GPU::draw_mode5()
{
    u8 palette_index;                  // in mode 4 each pixel uses 1 byte 
    u16 color;                         // the color located at pallette_ram[palette_index]
    u32 pal_ptr = MEM_VRAM_START;      // address of current palette

    // page 2 starts at 0x600A000
    if (stat->dispcnt.ps)
        pal_ptr += 0xA000;

    // mode 5 has resolution of 128 x 160
    for (int y = 0; y < 128; ++y)
    {
        for (int x = 0; x < 160; ++x)
        {
            palette_index = mem->read_u8_unprotected(pal_ptr++);
            // multiply by sizeof(u16) because each entry in palram is 2 bytes
            color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + (palette_index * sizeof(u16)));
            // add current pixel in argb format to pixel array
            screen_buffer[y][x] = u16_to_u32_color(color);
        }
    }
}

// draws regular background
// bg - background index (0-3)
void GPU::draw_reg_background(int bg)
{
    return;
    // boundaries for window
    u16 xmax, xmin;
    u8  ymax, ymin;

    // initially all values are in bounds
    xmin = ymin = 0;
    xmax = 0xFFFF;
    ymax = 0xFF;

    // win0 enabled & bg is in content of win0
    if ((stat->dispcnt.win_enabled & 0x1) && ((mem->read_u16_unprotected(REG_WININ) >> bg) & 1))
    {
        xmax = win0rr; // right
        xmin = win0ll; // left
        ymax = win0bb; // bottom
        ymin = win0tt; // top
    }

    // win1 enabled & bg is in content of win1
    if ((stat->dispcnt.win_enabled & 0x2) && ((mem->read_u16_unprotected(REG_WININ) >> bg + 8) & 1))
    {
        xmax = win1rr; // right
        xmin = win1ll; // left
        ymax = win1bb; // bottom
        ymin = win1tt; // top
    }

    // initial address of background tileset
    u32 tileset_address = MEM_VRAM_START + CHARBLOCK_LEN * stat->bg_cnt[bg].cbb;

    // initial address of background tilemap
    u32 start_tilemap_address = MEM_VRAM_START + SCREENBLOCK_LEN * stat->bg_cnt[bg].sbb;
    u32 tilemap_address = start_tilemap_address;

    // get width / height (in tiles) of background
    int width, height;
    switch(stat->bg_cnt[bg].size)
    {
        case 0:
            width  = 32;
            height = 32;
        break;
        
        case 1:
            width  = 64;
            height = 32;
        break;

        case 2:
            width  = 32;
            height = 64;
        break;

        case 3:
            width  = 64;
            height = 64;
        break;
    }

    // entire map (bigger than screen)
    u32 map[height * PX_IN_TILE_COL][width * PX_IN_TILE_ROW];
    memset(map, 1, sizeof(map));

    u16 screen_entry;    // contains tile index, flipping flags, and balbank for s-tiles
    u16 tilemap_index;   // index # of current tile
    u32 cur_screenblock_ptr; // address of palette entries of current tile
    u16 color;           // arbg color of current pixel
    u8 palette_index;    // index of entry in palette memory
    u8 palbank;          // palette bank (for s-tiles)
    int x, y;            // (x, y) coordinate of current pixel;

    u8 left_pixel;
    u8 right_pixel;
    u8 vflip, hflip; // vertical / horizontal flip tile

    // TODO - yikes...
    for (int ssy = 0; ssy < (height / 32); ++ssy)
    {
        for (int ssx = 0; ssx < (width / 32); ++ssx)
        {
            tilemap_address = start_tilemap_address + ssx * SCREENBLOCK_LEN + (2 * ssy * SCREENBLOCK_LEN);
            for (int h = 0; h < TILES_PER_SCREENBLOCK; ++h)
            {
                for (int w = 0; w < TILES_PER_SCREENBLOCK; ++w)
                {
                    screen_entry = mem->read_u16_unprotected(tilemap_address);
                    tilemap_index = screen_entry & 0x3FF; // bits 9-0 
                    hflip = (screen_entry >> 10) & 0x1;     // bit  10
                    vflip = (screen_entry >> 11) & 0x1;     // bit  11

                    if (stat->bg_cnt[bg].color_mode == 0) // s-tile (4bpp)
                    {
                        palbank = screen_entry >> 12 & 0xF; // bits F - C
                        cur_screenblock_ptr = tileset_address + S_SCREENENTRY_DEPTH * tilemap_index;
                        for (int i = 0; i < S_TILE_LEN; ++i)
                        {
                            // 256 bc each screenblock is 32 tiles, 32 * 8 = 256
                            x = ssx * 256 + w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
                            y = ssy * 256 + h * PX_IN_TILE_COL + (i / 4);

                            if (hflip)
                            {
                                x = x + (7 - (x % 8)) - (x % 8);
                            }

                            if (vflip)
                            {
                                y = y + (7 - (y % 8)) - (y % 8);
                            }

                            palette_index = mem->read_u8_unprotected(cur_screenblock_ptr++);
                            
                            left_pixel  = (palette_index >> 0) & 0xF;
                            right_pixel = (palette_index >> 4) & 0xF;

                            // add left, right pixel to screen buffer
                            // pixel value 0 is transparent, so only draw if not 0
                            if (left_pixel != 0)
                            {
                                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                                color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
                                map[y][x] = u16_to_u32_color(color);
                            }

                            // pixel value 0 is transparent, so only draw if not 0
                            if (right_pixel != 0)
                            {
                                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                                color = mem->read_u16_unprotected(MEM_PALETTE_RAM_START + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));

                                if (hflip)
                                    x -= 2;

                                map[y][x + 1] = u16_to_u32_color(color);
                            }
                        }

                    }
                    
                    else // d-tile (8bpp)
                    {
                        cur_screenblock_ptr = tileset_address + D_SCREENENTRY_DEPTH * tilemap_index;
                        for (int i = 0; i < D_TILE_LEN; i++)
                        {

                            palette_index = mem->read_u8_unprotected(cur_screenblock_ptr++);

                            // pixel value 0 is transparent, so only draw if not 0
                            if (palette_index == 0)
                                continue; 

                            // 256 bc each screenblock is 32 tiles, 32 * 8 = 256
                            x = ssx * 256 + w * PX_IN_TILE_ROW + (i % 8);
                            y = ssy * 256 + h * PX_IN_TILE_COL + (i / 8);

                            // multiply by sizeof(u16) because each entry in palram is 2 bytes
                            color = mem->read_u32_unprotected(MEM_PALETTE_RAM_START + palette_index * sizeof(u16));

                            map[y][x] = u16_to_u32_color(color);
                        }
                    }

                    tilemap_address += 2; // each tile is 2 bytes long
                }
            }
        }
    }

    // get vertical & horizontal offset
    u16 voff, hoff;
    switch (bg)
    {
        case 0:
            voff = mem->read_u16_unprotected(REG_BG0VOFS);
            hoff = mem->read_u16_unprotected(REG_BG0HOFS);
        break;

        case 1:
            voff = mem->read_u16_unprotected(REG_BG1VOFS);
            hoff = mem->read_u16_unprotected(REG_BG1HOFS);
        break;

        case 2:
            voff = mem->read_u16_unprotected(REG_BG2VOFS);
            hoff = mem->read_u16_unprotected(REG_BG2HOFS);
        break;

        case 3:
            voff = mem->read_u16_unprotected(REG_BG3VOFS);
            hoff = mem->read_u16_unprotected(REG_BG3HOFS);
        break;
    }

    // copy area of map that screen is over into screen buffer
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
    {
        for (int x = 0; x < SCREEN_WIDTH; ++x)
        {
            // belongs to winout
            if ((stat->dispcnt.win_enabled & 0x3) && ((mem->read_u16_unprotected(REG_WINOUT) >> bg) & 1))
            {
                // lies within bounds of other window(s)
                if ((x < win0rr && x >= win0ll) && (y >= win0tt && y < win0bb)) continue;
                if ((x < win1rr && x >= win1ll) && (y >= win1tt && y < win1bb)) continue;
            }

            // does not belong to winout
            else
            {
                // out of window bounds
                if (x  > xmax || y  > ymax) continue;
                if (x <= xmin || y <= ymin) continue;
            }

            // modulo mapsize to allow wrapping
            if (map[(y + voff) % (height * PX_IN_TILE_COL)][(x + hoff) % (width * PX_IN_TILE_ROW)] != TRANSPARENT)
                screen_buffer[y][x] = map[(y + voff) % (height * PX_IN_TILE_COL)][(x + hoff) % (width * PX_IN_TILE_ROW)];
        }
    }
}

void GPU::draw_affine_background(int bg)
{
    return;
    std::cout << "aff bg\n";
    // initial address of background tileset
    u32 start_tileset_ptr = MEM_VRAM_START + CHARBLOCK_LEN * stat->bg_cnt[bg].cbb;

    // initial address of background tilemap
    u32 start_tilemap_ptr = MEM_VRAM_START + SCREENBLOCK_LEN * stat->bg_cnt[bg].sbb;
    u32 tilemap_ptr = start_tilemap_ptr;

    // get width / height (in tiles) of background
    int width, height;
    switch(stat->bg_cnt[bg].size)
    {
        case 0:
            width  = 16;
            height = 16;
        break;
        
        case 1:
            width  = 32;
            height = 32;
        break;

        case 2:
            width  = 64;
            height = 64;
        break;

        case 3:
            width  = 128;
            height = 128;
        break;
    }

    // entire map (bigger than screen)
    u32 map[height * PX_IN_TILE_COL][width * PX_IN_TILE_ROW];
    memset(map, 1, sizeof(map));

    u8 map_index;        // index # of current tile
    u32 tileset_ptr; // address of palette entries of current tile
    u16 color;           // arbg color of current pixel
    u8 palette_index;    // index of entry in palette memory
    int x, y;            // (x, y) coordinate of current pixel;

    for (int h = 0; h < height; ++h)
    {
        for (int w = 0; w < width; ++w)
        {
            map_index = mem->read_u8(tilemap_ptr++);
            
            // start address to current tile's palette 
            tileset_ptr = start_tileset_ptr + D_SCREENENTRY_DEPTH * map_index;

            // all affine BG are 256 color mode (D-tile)
            for (int i = 0; i < D_TILE_LEN; i++)
            {
                palette_index = mem->read_u8_unprotected(tileset_ptr++);

                // pixel value 0 is transparent, so only draw if not 0
                if (palette_index == 0)
                    continue; 

                // get x, y coordinate
                x = w * PX_IN_TILE_ROW + (i % 8);
                y = h * PX_IN_TILE_COL + (i / 8);

                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                color = mem->read_u32_unprotected(MEM_PALETTE_RAM_START + palette_index * sizeof(u16));

                map[y][x] = u16_to_u32_color(color);
            }
        }
    }

    s32 dx, dy; // displacement vector
    float pa, pb, pc, pd; // P matrix
    switch (bg)
    {
        case 2:
            dx = (s32) mem->read_u32_unprotected(REG_BG2X);
            dy = (s32) mem->read_u32_unprotected(REG_BG2Y);

            pa = (s16) mem->read_u16_unprotected(REG_BG2PA) / 256.0; 
            pb = (s16) mem->read_u16_unprotected(REG_BG2PB) / 256.0;
            pc = (s16) mem->read_u16_unprotected(REG_BG2PC) / 256.0;
            pd = (s16) mem->read_u16_unprotected(REG_BG2PD) / 256.0;
        break;

        case 3:
            dx = (s32) mem->read_u32_unprotected(REG_BG3X);
            dy = (s32) mem->read_u32_unprotected(REG_BG3Y);

            pa = (s16) mem->read_u16_unprotected(REG_BG3PA) / 256.0; 
            pb = (s16) mem->read_u16_unprotected(REG_BG3PB) / 256.0;
            pc = (s16) mem->read_u16_unprotected(REG_BG3PC) / 256.0;
            pd = (s16) mem->read_u16_unprotected(REG_BG3PD) / 256.0;
        break;
    }

    // remove fractional portion from displacement (24.8f)
    dx >>= 8;
    dy >>= 8;

    //std::cout << (int) dx << " " << (int) dy << "\n";

    // copy area of map that screen is over into screen buffer

    // wrapped
    if (stat->bg_cnt[bg].affine_wrap == 1)
    {
        for (int y = 0; y < SCREEN_HEIGHT; ++y)
        {
            for (int x = 0; x < SCREEN_WIDTH; ++x)
            {
                // modulo mapsize to allow wrapping
                if (map[(y + dy) % (height * PX_IN_TILE_COL)][(x + dx) % (width * PX_IN_TILE_ROW)] != TRANSPARENT)
                    screen_buffer[y][x] = map[(y) % (height * PX_IN_TILE_COL)][(x) % (width * PX_IN_TILE_ROW)];
            }
        }
    }

    // no wrap
    else
    {
        for (int y = 0; y < SCREEN_HEIGHT; ++y)
        {
            for (int x = 0; x < SCREEN_WIDTH; ++x)
            {
                // out of bounds
                if ((y + dy < 0) || (x + dx < 0))
                    continue;

                // modulo mapsize to allow wrapping
                if (map[y + dy][x + dx] != 0)
                    screen_buffer[y][x] = map[y + dy][x + dx];
            }
        }
    }

    // s16 x1, x2;
    // s8  y1, y2;

    // for (int y = 0; y < SCREEN_HEIGHT; ++y)
    // {
    //     for (int x = 0; x < SCREEN_WIDTH; ++x)
    //     {
    //         //std::cout << xx << " " << yy << "\n";
    //         y1 = y + dy;
    //         x1 = x + dx;

    //         // get new position of pixel after rot / scale
    //         // x2 = pa * (xx - x0) + pb * (yy - y0) + x0;
    //         // y2 = pc * (xx - x0) + pd * (yy - y0) + y0;

    //         x2 = pa * (x1) + pb * (y1);
    //         y2 = pc * (x1) + pd * (y1);

    //         // transfomation put pixel out of bounds
    //         // if ((x2 >= hwidth) || (y2 >= hheight))
    //         //     continue;
            
    //         // std::cout << (int) y1 << " " << (int) x1 << "\n";
    //         // std::cout << (int) y2 << " " << (int) x2 << "\n";

    //         // skip 0 pixel value (transparent)
    //         if (map[y2][x2] != 0)
    //             screen_buffer[y][x] = map[y2][x2];
    //     }
    // }

    //exit(0);
}

void GPU::draw_sprites()
{
    obj_attr attr;
    // TODO - priority
    for (int i = NUM_OBJS - 1; i >= 0; --i)
    {
        attr = objs[i];
        switch (attr.obj_mode)
        {
            case 0x0: // normal rendering
                draw_regular_sprite(attr);
                break;
            case 0x1: // affine & affine double
            case 0x3:
                draw_affine_sprite(attr);
                
                break;
            case 0x2: // hidden
                continue;
        }
    }
}

void GPU::draw_regular_sprite(obj_attr attr)
{
    //std::cout << "reg sprite\n";
    // boundaries for window
    u16 xmax, xmin;
    u8  ymax, ymin;

    // initially all values are in bounds
    xmin = ymin = 0;
    xmax = 0xFFFF;
    ymax = 0xFF;

    // win0 enabled & obj is in content of win0
    if ((stat->dispcnt.win_enabled & 0x1) && ((mem->read_u16_unprotected(REG_WININ) >> 4) & 1))
    {
        xmax = win0rr; // right
        xmin = win0ll; // left
        ymax = win0bb; // bottom
        ymin = win0tt; // top
    }

    // win1 enabled & obj is in content of win1
    if ((stat->dispcnt.win_enabled & 0x2) && ((mem->read_u16_unprotected(REG_WININ) >> 4 + 8) & 1))
    {
        xmax = win1rr; // right
        xmin = win1ll; // left
        ymax = win1bb; // bottom
        ymin = win1tt; // top
    }

    // x, y coordinate of top left of sprite
    u16 x0 = attr.x;
    u8  y0 = attr.y;

    // std::cout << "x: " << (int) x0 << " " << "y: " << (int) y0 << "\n";
    // std::cout << "w: " << (int) attr.width << " " << "h: " << (int) attr.height << "\n";
    
    u32 tile_ptr = LOWER_SPRITE_BLOCK + (attr.tileno * S_TILE_LEN);

    // temporary buffer to hold texture
    u32 sprite[attr.height * PX_IN_TILE_COL][attr.width * PX_IN_TILE_ROW];
    memset(sprite, 0x1, sizeof(sprite));

    u16 color;
    u8 palette_index; // nth entry in palram
    u16 x;
    u8 y;
    u8 left_pixel;
    u8 right_pixel;

    //draw sprite tile by tile
    for (int h = 0; h < attr.height; ++h)
    {
        for (int w = 0; w < attr.width; ++w)
        {
            if (attr.color_mode == 0) // 4 bits / pixel - s-tile
            {
                for (int i = 0; i < S_TILE_LEN; ++i)
                {
                    palette_index = mem->read_u8_unprotected(tile_ptr++);

                    x = w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
                    y = h * PX_IN_TILE_COL +     (i / 4);

                    left_pixel  = (palette_index >> 0) & 0xF;
                    right_pixel = (palette_index >> 4) & 0xF;
                    
                    // add left, right pixel to screen buffer
                    // pixel value 0 is transparent, so only draw if not 0
                    if (left_pixel != 0)
                    {
                        // multiply by sizeof(u16) because each entry in palram is 2 bytes
                        color = mem->read_u16_unprotected(SPRITE_PALETTE + left_pixel * sizeof(u16) + (attr.palbank * PALBANK_LEN));
                        sprite[y][x] = u16_to_u32_color(color);
                    }

                    // pixel value 0 is transparent, so only draw if not 0
                    if (right_pixel != 0)
                    {
                        // multiply by sizeof(u16) because each entry in palram is 2 bytes
                        color = mem->read_u16_unprotected(SPRITE_PALETTE + right_pixel * sizeof(u16) + (attr.palbank * PALBANK_LEN));
                        sprite[y][x + 1] = u16_to_u32_color(color);
                    }
                }
            }
            
            // 8 bits / pixel - d-tile
            else
            {
                for (int i = 0; i < D_TILE_LEN; i++)
                {
                    palette_index = mem->read_u8_unprotected(tile_ptr++);

                    // pixel value 0 is transparent, so only draw if not 0
                    if (palette_index == 0)
                        continue; 

                    x = x0 + w * PX_IN_TILE_ROW + (i % 8);
                    y = y0 + h * PX_IN_TILE_COL + (i / 8);

                    // multiply by sizeof(u16) because each entry in palram is 2 bytes
                    color = mem->read_u32_unprotected(SPRITE_PALETTE + palette_index * sizeof(u16));

                    screen_buffer[y][x] = u16_to_u32_color(color);
                }
            }
        }

        // 2d sprite mapping
        if (stat->dispcnt.obj_map_mode == 0) // 2d
        {
            if (attr.color_mode == 0)
                tile_ptr += (TILES_PER_SCREENBLOCK - attr.width) * S_TILE_LEN;
            else
                tile_ptr += (TILES_PER_SCREENBLOCK - attr.width) * D_TILE_LEN;
        }
    }

    //horizontal flip
    // if (attr.h_flip)
    // {
    //     u32 temp;
    //     for (int h = 0; h < attr.height * PX_IN_TILE_COL; ++h)
    //     {
    //         for (int w = 0; w < attr.width * 4; ++w) 
    //         {
    //             temp = sprite[h][w];

    //             sprite[h][w] = sprite[h][(attr.width * PX_IN_TILE_ROW) - w - 1];
    //             sprite[h][(attr.width * PX_IN_TILE_ROW) - w - 1] = temp;
    //         }
    //     }
    // }

    // // vertical flip
    // if (attr.v_flip) 
    // {
    //     u32 temp;
    //     for (int h = 0; h < attr.height * 4; ++h)
    //     {
    //         for (int w = 0; w < attr.width * PX_IN_TILE_ROW; ++w)
    //         {
    //             temp = sprite[h][w];

    //             sprite[h][w] = sprite[(attr.height * PX_IN_TILE_COL) - h - 1][w];
    //             sprite[(attr.height * PX_IN_TILE_COL) - h - 1][w] = temp;
    //         }
    //     }
    // }

    // mosaic
    if (attr.mosaic)
    {
        //std::cout << "Mosaic\n";
        // u16 mosaic = mem->read_u16_unprotected(REG_MOSAIC);
        
        // // horizontal / vertical stretch
        // // add one to get range 1 - 16
        // u8 wm = ((mosaic >>  8) & 0xF) + 1; // bits 8 - B
        // u8 hm = ((mosaic >> 12) & 0xF) + 1; // bits C - F

        // u32 temp;
        // for (int y = 0; y < height * PX_IN_TILE_COL; y += hm)
        // {
        //     for (int x = 0; x < width * PX_IN_TILE_ROW; x += wm)
        //     {
        //         //std::cout << (int) x << "\n";
        //         temp = screen_buffer[y0 + y][x0 + x];

        //         // fill wm more x values with the top left
        //         for (int xi = 0; xi < wm; ++xi)
        //         {
        //             // make sure it doesn't go past sprite's bounds
        //             //if (x0 + x + xi < (x0 + width * PX_IN_TILE_ROW))
        //             //std::cout << (int) x0 + x + xi << "\n";
        //                 screen_buffer[y0 + y][x0 + x + xi] = temp;
        //         }
                    

        //         // fill hm more y values with the top left    
        //         for (int yi = 0; yi < hm; ++yi)
        //         {
        //             //std::cout << (int) y0 + y + yi << "\n";
        //            //if (y0 + y + yi < (y0 + height * PX_IN_TILE_COL))
        //                 screen_buffer[y0 + y + yi][x0 + x] = temp;
        //         }
        //     }
        //     //std::cout << "\n";
        // }
    }

    // copy sprite to screen buffer
    for (int h = 0; h < attr.height * 8; ++h)
    {
        for (int w = 0; w < attr.width * 8; ++w)
        {
            // mask to keep x, y in 9, 8 bit range, respectively
            x = (x0 + w) & 0x1FF;
            y = (y0 + h) & 0xFF;

            if (x > SCREEN_WIDTH || y > SCREEN_HEIGHT)
                continue;
            
            // belongs to winout
            if ((stat->dispcnt.win_enabled & 0x3) && ((mem->read_u16_unprotected(REG_WINOUT) >> 4) & 1))
            {
                // lies within bounds of other window(s)
                if ((x < win0rr && x >= win0ll) && (y >= win0tt && y < win0bb)) continue;
                if ((x < win1rr && x >= win1ll) && (y >= win1tt && y < win1bb)) continue;
            }

            // does not belong to winout
            else
            {
                // out of window bounds
                if (x >  xmax || y  > ymax) continue;
                if (x <= xmin || y <= ymin) continue;
            }

            if (sprite[h][w] == TRANSPARENT)
                continue;

            screen_buffer[y][x] = sprite[h][w];
        }
    }
}

void GPU::draw_affine_sprite(obj_attr attr)
{
    return;
    std::cout << "aff sprite\n";
    // get affine matrix
    u32 oam_addr = MEM_OAM_START + attr.affine_index * 32; // each affine entry is 32 bytes across

    // transform P matrix from 8.8f to float
    // P = [pa pb]
    //     [pc pd]
    float pa = (s16) mem->read_u16(oam_addr +  0x6) / 256.0; 
    float pb = (s16) mem->read_u16(oam_addr +  0xE) / 256.0;
    float pc = (s16) mem->read_u16(oam_addr + 0x16) / 256.0;
    float pd = (s16) mem->read_u16(oam_addr + 0x1E) / 256.0;

    u32 tile_ptr = LOWER_SPRITE_BLOCK + (attr.tileno * S_TILE_LEN);

    // temporary buffer to hold texture
    u32 sprite[attr.height * PX_IN_TILE_COL][attr.width * PX_IN_TILE_ROW];
    memset(sprite, 1, sizeof(sprite));

    u16 color;
    u8 palette_index; // nth entry in palram
    u8 left_pixel, right_pixel;

    // x, y coordinate of sprite texture before rot/scale
    u16 x;
    u8  y;

    // draw sprite tile by tile
    for (int h = 0; h < attr.height; ++h)
    {
        for (int w = 0; w < attr.width; ++w)
        {
            if (attr.color_mode == 0) // 4 bits / pixel - s-tile
            {
                for (int i = 0; i < S_TILE_LEN; ++i)
                {
                    palette_index = mem->read_u8_unprotected(tile_ptr++);
                    x = w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
                    y = h * PX_IN_TILE_COL +     (i / 4);
                    
                    left_pixel  = (palette_index >> 0) & 0xF;
                    right_pixel = (palette_index >> 4) & 0xF;
                    
                    // add left, right pixel to screen buffer
                    // pixel value 0 is transparent, so only draw if not 0
                    if (left_pixel != 0)
                    {
                        // multiply by sizeof(u16) because each entry in palram is 2 bytes
                        color = mem->read_u16_unprotected(SPRITE_PALETTE + left_pixel * sizeof(u16) + (attr.palbank * PALBANK_LEN));
                        sprite[y][x] = u16_to_u32_color(color);
                    }

                    // pixel value 0 is transparent, so only draw if not 0
                    if (right_pixel != 0)
                    {
                        // multiply by sizeof(u16) because each entry in palram is 2 bytes
                        color = mem->read_u16_unprotected(SPRITE_PALETTE + right_pixel * sizeof(u16) + (attr.palbank * PALBANK_LEN));
                        sprite[y][x + 1] = u16_to_u32_color(color);
                    }
                }
            }
            
            // 8 bits / pixel - d-tile
            else
            {
                for (int i = 0; i < D_TILE_LEN; i++)
                {
                    palette_index = mem->read_u8_unprotected(tile_ptr++);

                    // pixel value 0 is transparent, so only draw if not 0
                    if (palette_index == 0)
                        continue; 

                    x = w * PX_IN_TILE_ROW + (i % 8);
                    y = h * PX_IN_TILE_COL + (i / 8);

                    // multiply by sizeof(u16) because each entry in palram is 2 bytes
                    color = mem->read_u32_unprotected(SPRITE_PALETTE + palette_index * sizeof(u16));

                    sprite[y][x] = u16_to_u32_color(color);
                }
            }

            // 2d sprite mapping
            if (stat->dispcnt.obj_map_mode) // 2d
            {
                if (attr.color_mode == 0)
                    tile_ptr += (TILES_PER_SCREENBLOCK - attr.width) * S_TILE_LEN;
                else
                    tile_ptr += (TILES_PER_SCREENBLOCK - attr.width) * D_TILE_LEN;
            }
        }
    }

    // half width, height (in pixels) of object screen canvas
    int hwidth  = attr.width  * 4; 
    int hheight = attr.height * 4; 

    // coordinate of center of screen space (q) and texture space (p)
    u16 qx0, px0;
    u8  qy0, py0;
    
    px0 = hwidth;
    py0 = hheight;

    // double flag set
    if (attr.obj_mode == 3)
    {
        // double available canvas space
        hwidth  *= 2;
        hheight *= 2;

        qx0 = attr.x + attr.width  * 8;
        qy0 = attr.y + attr.height * 8;
    }

    // double flag not set
    else
    {
        qx0 = attr.x + attr.width  * 4;
        qy0 = attr.y + attr.height * 4;
    }
    
    // coordinate of texture pixel after rot/scale
    int px, py;

    // loop through canvas space
    for (int iy = -hheight; iy < hheight; ++iy)
    {
        for (int ix = -hwidth; ix < hwidth; ++ix)
        {
            // get new position of pixel after rot / scale
            px = (pa * ix + pb * iy);
            py = (pc * ix + pd * iy);

            // transfomation put pixel out of bounds
            if ((px0 + px >= attr.width * PX_IN_TILE_ROW) || (py0 + py >= attr.height * PX_IN_TILE_COL))
                continue;
            
            if (px0 + px < 0 || py0 + py < 0)
                continue;
            
            if (qy0 + iy < 0 || qx0 + ix < 0)
                continue;
            
            if (qy0 + iy >= SCREEN_HEIGHT || qx0 + ix >= SCREEN_WIDTH)
                continue;
            
            // skip 0 pixel value (transparent)
            if (sprite[py0 + py][px0 + px] != TRANSPARENT)
                screen_buffer[qy0 + iy][qx0 + ix] = sprite[py0 + py][px0 + px];
        }
    }
}

// fills the objs data structure every frame an object needs to be drawn
void GPU::update_attr()
{
    u32 oam_ptr;
    u16 attr0, attr1, attr2;
    u8 i;

    // loop through all oam indices that are ready to be updated
    while (!stat->oam_update.empty())
    {
        i = stat->oam_update.front();
        //std::cout << (int) i << "\n";
        // get start address of dequeued oam entry
        oam_ptr = MEM_OAM_START + i * 8; // each entry is 8 bytes long

        attr0 = mem->read_u16_unprotected(oam_ptr + 0);
        attr1 = mem->read_u16_unprotected(oam_ptr + 2);
        attr2 = mem->read_u16_unprotected(oam_ptr + 4);

        objs[i].y            = attr0 >>  0 & 0xFF;
        objs[i].obj_mode     = attr0 >>  8 & 0x3;
        objs[i].gfx_mode     = attr0 >> 10 & 0x3;
        objs[i].mosaic       = attr0 >> 12 & 0x1;
        objs[i].color_mode   = attr0 >> 13 & 0x1;
        objs[i].size         = attr0 >> 14 & 0x3;

        objs[i].x            = attr1 >>  0 & 0x1FF;
        objs[i].affine_index = attr1 >>  9 & 0x1F;
        objs[i].h_flip       = attr1 >> 12 & 0x1;
        objs[i].v_flip       = attr1 >> 13 & 0x1;
        objs[i].shape        = attr1 >> 14 & 0x3;

        objs[i].tileno       = attr2 >>  0 & 0x3FF;
        objs[i].priority     = attr2 >> 10 & 0x3;
        objs[i].palbank      = attr2 >> 12 & 0xF;

        // get actual dimensions of sprite
        switch (objs[i].size)
        {
            case 0:
                switch (objs[i].size)
                {
                    case 0: objs[i].width =  8; objs[i].height =  8; break;
                    case 1: objs[i].width = 16; objs[i].height = 16; break;
                    case 2: objs[i].width = 32; objs[i].height = 32; break;
                    case 3: objs[i].width = 64; objs[i].height = 64; break;
                }
                break;

            case 1:
                switch (objs[i].size)
                {
                    case 0: objs[i].width = 16; objs[i].height =  8; break;
                    case 1: objs[i].width = 32; objs[i].height =  8; break;
                    case 2: objs[i].width = 32; objs[i].height = 16; break;
                    case 3: objs[i].width = 64; objs[i].height = 32; break;
                }
                break;

            case 2:
                switch (objs[i].size)
                {
                    case 0: objs[i].width =  8; objs[i].height = 16; break;
                    case 1: objs[i].width =  8; objs[i].height = 32; break;
                    case 2: objs[i].width = 16; objs[i].height = 32; break;
                    case 3: objs[i].width = 32; objs[i].height = 64; break;
                }
                break;
        }

        stat->oam_update.pop();
    }
}

// given a 16 bit GBA color, make it a 32 bit SDL color
inline u32 u16_to_u32_color (u16 color_u16)
{
    u8 a = 0x1F;
    u32 r, g, b;
    u32 color = 0; // alpha value 255 ?

    r = color_u16 & 0x1F; color_u16 >>= 5; // bits  0 - 5
    g = color_u16 & 0x1F; color_u16 >>= 5; // bits  6 - 10
    b = color_u16 & 0x1F; color_u16 >>= 5; // bits 11 - 15

    color |= (r << 19) | (g << 11) | (b << 3);
    return color;
}




// attempt at trying to draw a sprite not tile by tile
// for (u8 hh = 0; hh < height * PX_IN_TILE_COL; ++hh)
// {
//     tile_ptr = base_tile_addr + hh * PX_IN_TILE_COL * width;
//     std::cout << std::hex << tile_ptr << "\n";
//     for (u16 ww = 0; ww < width * PX_IN_TILE_COL; ++ww)
//     {
//         //std::cout << "x: " << ww << " " << "y: " << (int) hh << "\n";
//         palette_index = mem->read_u8_unprotected(tile_ptr + (ww / 8) * 32);
//         // x = start_x + w * PX_IN_TILE_ROW + 2 * (i % 4); // s-tiles get left/right px in one read 
//         // y = start_y + h * PX_IN_TILE_COL + (i / 4);

//         u8 left_pixel = palette_index & 0xF;
//         u8 right_pixel = (palette_index >> 4) & 0xF;
        
//         // add left, right pixel to screen buffer
//         // pixel value 0 is transparent, so only draw if not 0
//         if (left_pixel != 0)
//         {
//             // multiply by sizeof(u16) because each entry in palram is 2 bytes
//             color = mem->read_u16_unprotected(SPRITE_PALETTE + left_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
//             screen_buffer[y0 + hh][x0 + ww] = u16_to_u32_color(color);
//         }

//         // pixel value 0 is transparent, so only draw if not 0
//         if (right_pixel != 0)
//         {
//             // multiply by sizeof(u16) because each entry in palram is 2 bytes
//             color = mem->read_u16_unprotected(SPRITE_PALETTE + right_pixel * sizeof(u16) + (palbank * PALBANK_LEN));
//             screen_buffer[y0 + hh][x0 + ww + 1] = u16_to_u32_color(color);
//         }

//         ww++;

//         //base_tile_addr += s_tile ? S_TILE_LEN : D_TILE_LEN;
//     }
// }