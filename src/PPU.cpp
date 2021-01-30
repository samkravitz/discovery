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

#include "PPU.h"

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
#define TRANSPARENT 0x8000

u32 U16ToU32Color(u16);

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

    Reset();
}

PPU::~PPU()
{
    LOG("PPU: Shutdown\n");
    SDL_Quit();
}

void PPU::Reset()
{
    cycles    = 0;
    scanline  = 0;
    frame     = 0;
    fps       = 0;
    old_time  = clock();

    memset(screen_buffer, 0, sizeof(screen_buffer));

    // zero oam data structure
    for (int i = 0; i < NUM_OBJS; ++i)
    {
        objs[i].obj_mode     = 2; // hidden

        objs[i].x            = 0;
        objs[i].y            = 0;
        objs[i].x0           = 0;
        objs[i].y0           = 0;
        objs[i].gfx_mode     = 0;
        objs[i].mosaic       = 0;
        objs[i].color_mode   = 0;
        objs[i].size         = 0;
        objs[i].affine_index = 0;
        objs[i].h_flip       = 0;
        objs[i].v_flip       = 0;
        objs[i].shape        = 0;
        objs[i].tileno       = 0;
        objs[i].priority     = 0;
        objs[i].palbank      = 0;
        objs[i].width        = 0;
        objs[i].height       = 0;
        objs[i].hwidth       = 0;
        objs[i].hheight      = 0;

        objs[i].pa           = 0.0;
        objs[i].pb           = 0.0;
        objs[i].pc           = 0.0;
        objs[i].pd           = 0.0;
    }
}

// 1 clock cycle of the PPU
void PPU::Tick()
{
    cycles++;

    // start HBlank
    if (cycles == HDRAW)
    {
        if (scanline < SCREEN_HEIGHT)
            RenderScanline();

        stat->DisplayStatus.in_hBlank = true;

        // fire HBlank interrupt if necessary
        if (stat->DisplayStatus.hbi)
        {
            mem->memory[REG_IF] |= IRQ_HBLANK;
            //LOG(LogLevel::Debug, "HBlank interrupt\n");
        }
            

        // check for DMA HBlank requests
        // TODO - Don't fire DMA Hblank in VBlank
        for (int i = 0; i < 4; ++i)
        {
            if (mem->dma[i].enable && mem->dma[i].mode == 2) // start at HBLANK
            {
                mem->_Dma(i);
                LOG(LogLevel::Debug, "DMA {} HBLANK\n", i);
            }
        }
        
        // start VBlank
        if (scanline == VDRAW)
        {
            Render();
            stat->DisplayStatus.in_vBlank = true;

            // fire Vblank interrupt if necessary
            if (stat->DisplayStatus.vbi)
            {
                //LOG(LogLevel::Debug, "VBlank interrupt\n");
                mem->memory[REG_IF] |= IRQ_VBLANK;
            }
                
            // check for DMA VBLANK requests
            for (int i = 0; i < 4; ++i)
            {
                if (mem->dma[i].enable && mem->dma[i].mode == 1) // start at VBLANK
                {
                    mem->_Dma(i);
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
            stat->DisplayStatus.in_vBlank = false;
            scanline = 0;
            stat->scanline = 0;
        }

        else
        {
            scanline++;
            stat->scanline++;
        }

        // scanline has reached trigger value
        if (scanline == stat->DisplayStatus.vct)
        {
            // set trigger status
            stat->DisplayStatus.vcs = 1;
            
            // scanline interrupt is triggered if requested
            if (stat->DisplayStatus.vci)
            {
                mem->memory[REG_IF] |= IRQ_VCOUNT;
                //std::cout << "Scanline interrupt\n";
            }
                
        }
        
        // scanline is not equal to trigger value, reset this bit
        else
        {
            stat->DisplayStatus.vcs = 0;
        }
        
        cycles = 0;
        stat->DisplayStatus.in_hBlank = false;
    }
}

void PPU::Render()
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

    // update objs data structure
    if (stat->DisplayControl.obj_enabled)
        UpdateAttr();

    // zero screen buffer for next frame
    if (stat->DisplayControl.fb)
        memset(screen_buffer, 0xFF, sizeof(screen_buffer)); // white
    else
        memset(screen_buffer, 0, sizeof(screen_buffer));    // black
}

void PPU::RenderScanline()
{
    std::memset(scanline_buffer, 0, sizeof(scanline_buffer));

    switch (stat->DisplayControl.mode)
    {
        case 0:
            for (int i = 3; i >= 0; --i) // bg0 - bg3
            {
                if (stat->BgControl[i].enabled)
                    RenderScanlineText(i);
            }

            break;
        case 3:
        case 4:
        case 5:
            RenderScanlineBitmap(stat->DisplayControl.mode);
            break;
    }

    std::memcpy(&screen_buffer[scanline * SCREEN_WIDTH], scanline_buffer, sizeof(scanline_buffer));

    if (stat->DisplayControl.obj_enabled)
    {
        RenderScanlineObj();
        std::memcpy(&screen_buffer[scanline * SCREEN_WIDTH], obj_scanline_buffer, sizeof(obj_scanline_buffer));
    }
    
}

void PPU::RenderScanlineText(int bg)
{
    auto &bgcnt = stat->BgControl[bg];

    // vertical, horizontal offset
    //int voff = mem->Read32Unsafe(REG)
    // screen position
    //int px, py = (scanline + bgcnt.voff) % bgcnt.height;
    int map_x, map_y = scanline;
    
    int tile_index;
    int screenblock = 30;
    u32 sb_address = MEM_VRAM_START + 0x800 * screenblock;
    int tile_x, tile_y; // tile x, y coordinate (in screenblock)
    for (int x = 0; x < SCREEN_WIDTH; ++x)
    {
        //px = (x + bgcnt.hoff) % bgcnt.width;
        map_x = x;

        tile_x = map_x / 8;
        tile_y = map_y / 8;

        u16 screenentry = mem->Read16(sb_address + (2 * (tile_y * 32 + tile_x)));
        int se_index = screenentry & 0x3FF;

        u32 tile_addr = MEM_VRAM_START + 0x20 * se_index;
        int palbank = screenentry >> 12 & 0xF;

        int pixel = GetBGPixel4BPP(tile_addr, palbank, map_x % 8, map_y % 8);

        if (pixel != TRANSPARENT)
            scanline_buffer[x] = U16ToU32Color(pixel);
    }

}

// render the current scanline for bitmap modes
void PPU::RenderScanlineBitmap(int mode)
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
                pixel = mem->Read16Unsafe(pal_ptr); pal_ptr += 2;
                scanline_buffer[i] = U16ToU32Color(pixel);
            }
            break;
        
        case 4:
            pal_ptr = MEM_VRAM_START + (scanline * SCREEN_WIDTH);

            // page 2 starts at 0x600A000
            if (stat->DisplayControl.ps)
                pal_ptr += 0xA000;

            for (int i = 0; i < SCREEN_WIDTH; ++i)
            {
                palette_index = mem->Read8Unsafe(pal_ptr++);
                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                pixel = mem->Read16Unsafe(MEM_PALETTE_RAM_START + (palette_index * sizeof(u16)));
                scanline_buffer[i] = U16ToU32Color(pixel);
            }

            break;
        
        case 5:
            if (scanline >= 128) return; // mode 5 has 160 x 128 resolution

            pal_ptr = MEM_VRAM_START + (scanline * 160);

            // page 2 starts at 0x600A000
            if (stat->DisplayControl.ps)
                pal_ptr += 0xA000;

            for (int i = 0; i < 160; ++i)
            {
                palette_index = mem->Read8Unsafe(pal_ptr++);
                // multiply by sizeof(u16) because each entry in palram is 2 bytes
                pixel = mem->Read16Unsafe(MEM_PALETTE_RAM_START + (palette_index * sizeof(u16)));
                scanline_buffer[i] = U16ToU32Color(pixel);
            }

            break;
    }
}

void PPU::RenderScanlineObj()
{
    // zero obj scanline
    std::memset(obj_scanline_buffer, 0, sizeof(obj_scanline_buffer));

    ObjAttr *attr;
    u16 pixel;

    // loop through all objs
    for (int i = NUM_OBJS - 1; i >= 0; --i)
    {
        attr = &objs[i];

        // skip hidden object
        if (attr->obj_mode == 2)
            continue;

        // // obj exists outside current scanline
        if (scanline < attr->y0 || scanline >= attr->y0 + attr->height)
            continue;
        
        int px0 = attr->hwidth; // center of sprite texture
        int qx0 = attr->x;      // center of sprite screen space

        // x, y coordinate of texture after transformation
        int px, py;
        int iy = scanline - attr->height;

        for (int ix = -attr->hwidth; ix < attr->hwidth; ++ix)
        {
            px = px0 + ix;
            py = scanline - attr->y0;

            // transform affine & double wide affine
            if (attr->obj_mode == 1 || attr->obj_mode == 3)
            {
                px = (attr->pa * ix + attr->pb * iy) + (attr->width  / 2);
                py = (attr->pc * ix + attr->pd * iy) + (attr->height / 2);
            }

            // horizontal / vertical flip
            if (attr->h_flip) px = attr->width  - px - 1;
            if (attr->v_flip) py = attr->height - py - 1;
            
            // transformed coordinate is out of bounds
            if (px >= attr->width || py >= attr->height) continue;
            if (px < 0            || py < 0            ) continue;
            if (qx0 + ix < 0      || qx0 + ix >= 240   ) continue;
            
            int tile_x  = px % 8; // x coordinate of pixel within tile
            int tile_y  = py % 8; // y coordinate of pixel within tile
            int block_x = px / 8; // x coordinate of tile in vram
            int block_y = py / 8; // y coordinate of tile in vram

            int tileno = attr->tileno;
            int pixel;

            if (attr->color_mode == 1) // 8bpp
            {
                if (stat->DisplayControl.obj_map_mode == 1) // 1d
                {
                    tileno += block_y * (attr->width / 4);
                }

                else // 2d
                {
                    tileno = (tileno & ~1) + block_y * 32;
                }
                
                tileno += block_x * 2;

                pixel = GetObjPixel8BPP(LOWER_SPRITE_BLOCK + tileno * 32, tile_x, tile_y);
            }

            else // 4bpp
            {
                if (stat->DisplayControl.obj_map_mode == 1) // 1d
                {
                    tileno += block_y * (attr->width / 8);
                }

                else // 2d
                {
                    tileno += block_y * 32;
                }

                tileno += block_x;

                pixel = GetObjPixel4BPP(LOWER_SPRITE_BLOCK + tileno * 32, attr->palbank, tile_x, tile_y);
            }
            
            if (pixel != TRANSPARENT) obj_scanline_buffer[qx0 + ix] = U16ToU32Color(pixel);
        }
    }
}

// TODO - add queue that holds index of every obj to be drawn this frame
// fills the objs data structure every frame an object needs to be drawn
void PPU::UpdateAttr()
{
    u32 oam_ptr = MEM_OAM_START;
    u16 attr0, attr1, attr2;
    
    // loop through all objs
    for (int i = 0; i < NUM_OBJS; ++i)
    {   
        ObjAttr &obj = objs[i];

        attr0 = mem->Read16Unsafe(oam_ptr); oam_ptr += 2;
        attr1 = mem->Read16Unsafe(oam_ptr); oam_ptr += 2;
        attr2 = mem->Read16Unsafe(oam_ptr); oam_ptr += 4;

        obj.y0           = attr0 >>  0 & 0xFF;
        obj.obj_mode     = attr0 >>  8 & 0x3;
        obj.gfx_mode     = attr0 >> 10 & 0x3;
        obj.mosaic       = attr0 >> 12 & 0x1;
        obj.color_mode   = attr0 >> 13 & 0x1;
        obj.shape        = attr0 >> 14 & 0x3;

        obj.x0           = attr1 >>  0 & 0x1FF;
        obj.affine_index = attr1 >>  9 & 0x1F;
        obj.h_flip       = attr1 >> 12 & 0x1;
        obj.v_flip       = attr1 >> 13 & 0x1;
        obj.size         = attr1 >> 14 & 0x3;

        obj.tileno       = attr2 >>  0 & 0x3FF;
        obj.priority     = attr2 >> 10 & 0x3;
        obj.palbank      = attr2 >> 12 & 0xF;

        if (obj.x0 >= SCREEN_WIDTH)  obj.x0 -= 512;
        if (obj.y0 >= SCREEN_HEIGHT) obj.y0 -= 256;

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
        obj.hwidth  = obj.width / 2;
        obj.hheight = obj.height / 2;

        // x, y of sprite origin
        obj.x = obj.x0 + obj.hwidth;
        obj.y = obj.y0 + obj.hheight;

        // get affine matrix if necessary
        if (obj.obj_mode == 1 || obj.obj_mode == 3) // affine
        {
            u32 matrix_ptr = MEM_OAM_START + obj.affine_index * 32; // each affine entry is 32 bytes across

            // transform P matrix from 8.8f to float
            // P = [pa pb]
            //     [pc pd]
            obj.pa = (s16) mem->Read16(matrix_ptr +  0x6) / 256.0; 
            obj.pb = (s16) mem->Read16(matrix_ptr +  0xE) / 256.0;
            obj.pc = (s16) mem->Read16(matrix_ptr + 0x16) / 256.0;
            obj.pd = (s16) mem->Read16(matrix_ptr + 0x1E) / 256.0;

            // double wide affine
            if (obj.obj_mode == 3)
            {
                obj.x += obj.width;
                obj.y += obj.height;
                
                obj.hwidth  *= 2;
                obj.hheight *= 2;
            }

            // make sure flips are set to zero
            obj.v_flip = 0;
            obj.h_flip = 0;
        }
    }
}

u16 PPU::GetObjPixel4BPP(u32 addr, int palbank, int x, int y)
{
    addr += (y * 4) + (x / 2);
    
    u16 palette_index = mem->Read8(addr);

    // use top nybble for odd x, even otherwise
    if (x & 1) { palette_index >>= 4; }

    palette_index &= 0xF;

    if (palette_index == 0) 
        return TRANSPARENT;

    return mem->Read16(SPRITE_PALETTE + palette_index * sizeof(u16) + (palbank * PALBANK_LEN));
}

u16 PPU::GetObjPixel8BPP(u32 addr, int x, int y)
{
    addr += (y * 8) + x;
    
    u16 palette_index = mem->Read8(addr);

    if (palette_index == 0) 
        return TRANSPARENT;

    return mem->Read16(SPRITE_PALETTE + palette_index * sizeof(u16));
}

u16 PPU::GetBGPixel4BPP(u32 addr, int palbank, int x, int y)
{
    addr += (y * 4) + (x / 2);
    
    u16 palette_index = mem->Read8(addr);

    // use top nybble for odd x, even otherwise
    if (x & 1) { palette_index >>= 4; }

    palette_index &= 0xF;

    if (palette_index == 0) 
        return TRANSPARENT;

    return mem->Read16(BG_PALETTE + palette_index * sizeof(u16) + (palbank * PALBANK_LEN));
}

u16 PPU::GetBGPixel8BPP(u32 addr, int x, int y)
{
    addr += (y * 8) + x;
    
    u16 palette_index = mem->Read8(addr);

    if (palette_index == 0) 
        return TRANSPARENT;

    return mem->Read16(BG_PALETTE + palette_index * sizeof(u16));
}

// given a 16 bit GBA color, make it a 32 bit SDL color
inline u32 U16ToU32Color (u16 color_u16)
{
    u8 a = 0x1F;
    u32 r, g, b;
    u32 color = 0; // alpha value 255 ?

    r = color_u16 & 0x1F; color_u16 >>= 5; // bits  0 - 5
    g = color_u16 & 0x1F; color_u16 >>= 5; // bits  6 - 10
    b = color_u16 & 0x1F; color_u16 >>= 5; // bits 11 - 15

    return r << 19 | g << 11 | b << 3;
}