/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: platform/wx/Frame.h
 * DATE: September 14, 2021
 * DESCRIPTION: Main window for WxWidgets
 */
#pragma once

#include <wx/wx.h>

#include "Panel.h"

class Frame : public wxFrame
{
public:
    Frame();
    Panel &getPanel() { return *panel; }
    
private:
    Panel *panel;
    
    void onFileExit(wxCommandEvent &);
    void onHelpAbout(wxCommandEvent &);
};