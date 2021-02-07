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

        stat->displaystat.in_hBlank = true;

        // fire HBlank interrupt if necessary
        if (stat->displaystat.hbi)
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
            stat->displaystat.in_vBlank = true;

            // fire Vblank interrupt if necessary
            if (stat->displaystat.vbi)
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
            stat->displaystat.in_vBlank = false;
            scanline = 0;
            stat->scanline = 0;
        }

        else
        {
            scanline++;
            stat->scanline++;
        }

        // scanline has reached trigger value
        if (scanline == stat->displaystat.vct)
        {
            // set trigger status
            stat->displaystat.vcs = 1;
            
            // scanline interrupt is triggered if requested
            if (stat->displaystat.vci)
            {
                mem->memory[REG_IF] |= IRQ_VCOUNT;
                //std::cout << "Scanline interrupt\n";
            }
                
        }
        
        // scanline is not equal to trigger value, reset this bit
        else
        {
            stat->displaystat.vcs = 0;
        }
        
        cycles = 0;
        stat->displaystat.in_hBlank = false;
    }
}

void PPU::Render()
{
    //std::cout << "Executing graphics mode: " << (int) (stat->dispcnt.mode) << "\n";

    // update objs data structure
    if (stat->dispcnt.obj_enabled)
    {
        UpdateAttr();
        RenderObj();
        //std::memcpy(&screen_buffer[scanline * SCREEN_WIDTH], obj_scanline_buffer, sizeof(obj_scanline_buffer));
    }
    
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

    // zero screen buffer for next frame
    if (stat->dispcnt.fb)
        memset(screen_buffer, 0xFF, sizeof(screen_buffer)); // white
    else
       memset(screen_buffer,    0, sizeof(screen_buffer));    // black
}

void PPU::RenderScanline()
{
    std::memset(scanline_buffer, 0, sizeof(scanline_buffer));

    switch (stat->dispcnt.mode)
    {
        case 0: // reg bg 0-3
            for (int i = 3; i >= 0; --i) // bg0 - bg3
            {
                if (stat->bgcnt[i].enabled)
                    RenderScanlineText(i);
            }

            break;
        case 1: // reg bg 0-1, aff bg 2
            if (stat->bgcnt[2].enabled) RenderScanlineAffine(2);
            if (stat->bgcnt[1].enabled) RenderScanlineText(1);
            if (stat->bgcnt[0].enabled) RenderScanlineText(0);

            break;
        case 2: // aff bg 2-3
            if (stat->bgcnt[3].enabled) RenderScanlineAffine(3);
            if (stat->bgcnt[2].enabled) RenderScanlineAffine(2);

            break;
        case 3:
        case 4:
        case 5:
            RenderScanlineBitmap(stat->dispcnt.mode);
            break;
    }

    std::memcpy(&screen_buffer[scanline], scanline_buffer, sizeof(scanline_buffer));
}

void PPU::RenderScanlineText(int bg)
{
    auto const &bgcnt = stat->bgcnt[bg];

    int pitch; // pitch of screenblocks


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

    for (int x = 0; x < SCREEN_WIDTH; ++x)
    {
        map_x = (x + bgcnt.hoff) % width;
        tile_x = map_x / 8; // 8 px per tile

        screenblock = bgcnt.sbb + ((tile_y / 32) * pitch + (tile_x / 32));
        sb_addr = MEM_VRAM_START + SCREENBLOCK_LEN * (bgcnt.sbb);
        se_index = screenblock * 1024 + (tile_y % 32) * 32 + (tile_x % 32);

        screenentry = mem->Read16(MEM_VRAM_START + 2 * se_index);
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
            pixel = GetBGPixel4BPP(tile_addr, palbank, grid_x, grid_y);
        }

        else // 8BPP
        {
            tile_addr = (MEM_VRAM_START + bgcnt.cbb * CHARBLOCK_LEN) + 0x40 * tile_id;
            pixel = GetBGPixel8BPP(tile_addr, grid_x, grid_y);
        }


        if (pixel != TRANSPARENT)
            scanline_buffer[x] = U16ToU32Color(pixel);
    }

}

// render the current scanline for affine bg modes
void PPU::RenderScanlineAffine(int bg)
{
    auto const &bgcnt = stat->bgcnt[bg];

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

    u32 dx_raw, dy_raw;
    float dx, dy; // displacement vector
    float pa, pb, pc, pd; // P matrix
    switch (bg)
    {
        case 2:
            dx_raw = mem->Read32(REG_BG2X);
            dy_raw = mem->Read32(REG_BG2Y);

            pa = (s16) mem->Read32(REG_BG2PA) / 256.0; 
            pb = (s16) mem->Read32(REG_BG2PB) / 256.0;
            pc = (s16) mem->Read32(REG_BG2PC) / 256.0;
            pd = (s16) mem->Read32(REG_BG2PD) / 256.0;
            break;

        case 3:
            dx_raw = mem->Read32(REG_BG3X);
            dy_raw = mem->Read32(REG_BG3Y);

            pa = (s16) mem->Read32(REG_BG3PA) / 256.0; 
            pb = (s16) mem->Read32(REG_BG3PB) / 256.0;
            pc = (s16) mem->Read32(REG_BG3PC) / 256.0;
            pd = (s16) mem->Read32(REG_BG3PD) / 256.0;
            break;
    }

    dx = (float) (dx_raw >> 8 & 0x7FFFF) + ((dx_raw & 0xFF) / 256.0);
    dy = (float) (dy_raw >> 8 & 0x7FFFF) + ((dy_raw & 0xFF) / 256.0);

    if (dx_raw & 0x8000000)
        dx *= -1.0;
    if (dy_raw & 0x8000000)
        dy *= -1.0;
    

    LOG("{} {} {} {}\n", (float) dx, dx_raw, (float) dy, dy_raw);
    

    int px0 = width / 2;
    int px, py;
    int x0 = dx, y0 = dy;

    // map position
    int map_x, map_y = scanline;

    // tile coordinates (in map)
    int tile_x, tile_y = map_y / 8; // 8 px per tile 
    
    int se_index;
    int pixel;
    u32 tile_addr;

    for (int x = 0; x < SCREEN_WIDTH; ++x)
    {
        px = pa * (x - x0) + pb * (map_y - y0) + x0;
        py = pc * (x - x0) + pd * (map_y - y0) + y0;

        // transformmed coordinate is out of bounds
        if (px >= SCREEN_WIDTH || py >= SCREEN_HEIGHT) continue;
        if (px < 0             || py < 0)              continue;
    
        map_x = x;
        tile_x = map_x / 8; // 8 px per tile

        se_index = mem->Read8((MEM_VRAM_START + bgcnt.sbb * SCREENBLOCK_LEN) + tile_y * (width / 8) + tile_x);
        tile_addr = (MEM_VRAM_START + bgcnt.cbb * CHARBLOCK_LEN) + (se_index * 0x40);

        // 8BPP only
        pixel = GetBGPixel8BPP(tile_addr, map_x % 8, map_y % 8);

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
            if (stat->dispcnt.ps)
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
            if (stat->dispcnt.ps)
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

void PPU::RenderObj()
{
    ObjAttr *attr;
    u16 pixel;

    while (!oam_update->empty())
    {
        attr = &objs[oam_update->top()];
        oam_update->pop();

        // skip hidden object
        if (attr->obj_mode == 2)
            continue;
        
        int px0 = attr->hwidth; // center of sprite texture
        int qx0 = attr->x;      // center of sprite screen space

        int py0 = attr->hheight;
        int qy0 = attr->y;

        // x, y coordinate of texture after transformation
        int px, py;
        int iy = scanline - attr->height;

        for (int iy = -attr->hheight; iy < attr->hheight; ++iy)
        {
            for (int ix = -attr->hwidth; ix < attr->hwidth; ++ix)
            {
                px = px0 + ix;
                py = py0 + iy;

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
                if (qy0 + iy < 0      || qy0 + iy >= 160   ) continue;
                
                int tile_x  = px % 8; // x coordinate of pixel within tile
                int tile_y  = py % 8; // y coordinate of pixel within tile
                int block_x = px / 8; // x coordinate of tile in vram
                int block_y = py / 8; // y coordinate of tile in vram

                int tileno = attr->tileno;
                int pixel;

                if (attr->color_mode == 1) // 8bpp
                {
                    if (stat->dispcnt.obj_map_mode == 1) // 1d
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
                    if (stat->dispcnt.obj_map_mode == 1) // 1d
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
                
                if (pixel != TRANSPARENT)
                    screen_buffer[qy0 + iy][qx0 + ix] = U16ToU32Color(pixel);
            }
        }
    }
}

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
                obj.x += obj.hwidth;
                obj.y += obj.hheight;
                
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