/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: platform/wx/Panel.cpp
 * DATE: September 14, 2021
 * DESCRIPTION: GBA "screen" in debug mode
 */
#include "Panel.h"

#include <iostream>
#include <wx/dcbuffer.h>
#include <wx/image.h>

#include "common.h"

Panel::Panel(wxWindow *parent) :
	wxPanel(parent, wx::IDP_PANEL)
{
	screen = SDL_CreateRGBSurface(0, 480, 320, 24, 0, 0, 0, 0);     

    // ensure the size of the wxPanel
    wxSize size(480, 320);
    
    SetMinSize(size);
    SetMaxSize(size);

	Bind(wxEVT_ERASE_BACKGROUND, [this](wxEraseEvent &event) { });
	Bind(wxEVT_PAINT, [this](wxPaintEvent &event)
	{
		render(event);
	});

	Bind(wxEVT_IDLE, [this](wxIdleEvent &event)
	{
		onIdle(event);
	});
}

Panel::~Panel()
{
    if (screen)
        SDL_FreeSurface(screen);
}

void Panel::render(wxPaintEvent &event)
{    
    if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);
    
    // create a bitmap from our pixel data
    wxBitmap bmp(wxImage(screen->w, screen->h, static_cast<unsigned char *>(screen->pixels), true));
    
    if (SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);
    
    // paint the screen
    wxBufferedPaintDC dc(this, bmp);
}

void Panel::onIdle(wxIdleEvent &event)
{
    if (SDL_MUSTLOCK(screen))
        SDL_LockSurface(screen);


    // paint the screen a solid blue
    for (int y = 0; y < 320; y++)
	{
        for (int x = 0; x < 480; x++)
		{
            wxUint8 *pixels = static_cast<wxUint8 *>(screen->pixels) + 
                              (y * screen->pitch) +
                              (x * screen->format->BytesPerPixel);

                pixels[0] = 0;
                pixels[1] = 0;
                pixels[2] = 0xff;
        }
    }
    
    // Unlock if needed
    if (SDL_MUSTLOCK(screen))
		SDL_LockSurface(screen);

    // refresh the panel
    Refresh(false);
    
    // throttle to keep from flooding the event queue
    wxMilliSleep(33);
}
