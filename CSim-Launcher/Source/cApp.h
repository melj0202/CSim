#pragma once
#include <wx/wxprec.h>
#include "_GLOBALS.h"
#include "wx/wx.h"

class cApp:public wxApp
{
public:
    cApp();
    ~cApp();
    virtual bool OnInit();
};

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
private:
    void OnHello(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    wxDECLARE_EVENT_TABLE();
};


enum
{
    ID_Hello = 1
};


