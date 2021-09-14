/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: platform/wx/Panel.h
 * DATE: September 14, 2021
 * DESCRIPTION: GBA "screen" in debug mode
 */
#pragma once

#include <wx/wx.h>

#include <SDL2/SDL.h>

class Panel : public wxPanel
{

public:
    Panel(wxWindow *parent);
	~Panel();

private:
    SDL_Surface *screen;

    void render(wxPaintEvent &);
    void onEraseBackground(wxEraseEvent &);
    void onIdle(wxIdleEvent &);
};