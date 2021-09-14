/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: platform/wx/app.h
 * DATE: September 14, 2021
 * DESCRIPTION: Main application container for the WxWidget window
 */
#pragma once

#include <wx/wx.h>

#include "Frame.h"

class App : public wxApp
{

public:
    bool OnInit();
    int OnRun();
    int OnExit();

private:
    Frame *frame;
};