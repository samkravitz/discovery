/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: platform/wx/main.cpp
 * DATE: September 14, 2021
 * DESCRIPTION: Driver code for discovery "Debug mode"
 * RESOURCE: http://code.technoplaza.net/wx-sdl/
 */
#include <cassert>
#include <SDL2/SDL.h>

#include "App.h"

wxIMPLEMENT_APP_NO_MAIN(App);

int main(int argc, char **argv)
{
    assert(SDL_Init(SDL_INIT_VIDEO) >= 0);
    return wxEntry(argc, argv);
}
