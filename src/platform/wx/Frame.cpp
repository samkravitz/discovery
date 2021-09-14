/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: platform/wx/Frame.cpp
 * DATE: September 14, 2021
 * DESCRIPTION: Main window for WxWidgets
 */
#include "Frame.h"

#include "common.h"

Frame::Frame()
{
    // Create the Frame
    Create(0, wx::IDF_FRAME, "discovery", wxDefaultPosition,
           wxDefaultSize, wxCAPTION | wxSYSTEM_MENU | 
           wxMINIMIZE_BOX | wxCLOSE_BOX);

    // create the main menubar
    wxMenuBar *mb = new wxMenuBar;
    
    // create the file menu
    wxMenu *fileMenu = new wxMenu;
    fileMenu->Append(wxID_EXIT, "E&xit");
    
    // add the file menu to the menu bar
    mb->Append(fileMenu, "&File");
    
    // create the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "About");
    
    // add the help menu to the menu bar
    mb->Append(helpMenu, "&Help");
    
    // add the menu bar to the Frame
    SetMenuBar(mb);
    
    // create the SDLPanel
    panel = new Panel(this);

    Bind(wxEVT_MENU, [=](wxCommandEvent &event)
    {
        wxMessageBox("Discovery GBA emulator\nCopyright (C) 2021 Sam Kravitz, Noah Bennett", "Discovery", wxOK | wxICON_INFORMATION);
    }, wxID_ABOUT);

    Bind(wxEVT_MENU, [this](wxCommandEvent &event)
    {
        Close();
    }, wxID_EXIT);
    
}
