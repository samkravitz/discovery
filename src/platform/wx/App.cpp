/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * Author: Sam Kravitz
 * 
 * FILE: platform/wx/app.cpp
 * DATE: September 14, 2021
 * DESCRIPTION: Main application container for the WxWidget window
 */
#include "App.h"

bool App::OnInit()
{
    frame = new Frame();
    frame->SetClientSize(480, 320);
    frame->Centre();
    frame->Show();
    SetTopWindow(frame);
    return true;
}

int App::OnRun()
{
    auto event = wxIdleEvent{};
    event.SetEventObject(&frame->getPanel());
    frame->getPanel().GetEventHandler()->AddPendingEvent(event);
    return wxApp::OnRun();
}

int App::OnExit()
{
    SDL_Quit();
    return wxApp::OnExit();
}
