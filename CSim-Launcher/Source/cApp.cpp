#include "cApp.h"

#include "../ThirdParty/wxWidgets/src/stc/scintilla/include/Platform.h"
#include "wx/display.h"
#include "wx/filepicker.h"
#include "wx/glcanvas.h"


cApp::cApp()
{
}


cApp::~cApp()
{
}

bool cApp::OnInit()
{
    //Captures the display dimensions of the main monitor
    wxRect disp_dimensions = wxDisplay().GetClientArea();

    MyFrame *frame = new MyFrame("CSim Launcher", wxPoint(50, 50), wxSize(WINDOW_WIDTH, WINDOW_HEIGHT));
    frame->CenterOnScreen();
    frame->Show(true);
    frame->SetSizeHints(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT);
    return true;
}
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(NULL, wxID_ANY, title, pos, size) {
    wxPanel *panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE );
    panel->SetSize(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    //Create title text and set its positon
    wxStaticText* text = new wxStaticText(panel, wxID_ANY, wxT("Welcome to the CSim Launcher!"),wxPoint(195, 30), wxSize(-1, -1), wxALIGN_CENTER_HORIZONTAL);

    //Assemble the List box and its choices
    wxArrayString choices;
    choices.push_back(wxString("Game of Life"));
    choices.push_back(wxString("Brains Brain"));
    choices.push_back(wxString("Life Without Death"));
    choices.push_back(wxString("Seeds"));
    choices.push_back(wxString("Highlife"));
    choices.push_back(wxString("Day and Night"));
    wxStaticText* mode_label = new wxStaticText(panel, wxID_ANY, wxT("Ruleset"));
    mode_label->SetPosition(wxPoint(420, 55));
    wxListBox* list_box = new wxListBox(panel, wxID_ANY);
    list_box->SetPosition(wxPoint(420, 80));
    list_box->SetSize(wxSize(145, 240));
    list_box->InsertItems(choices, 0);
    //Insert the launch button
    wxButton* launch_button = new wxButton(panel, wxID_ANY, wxT("Launch"));
    launch_button->SetPosition(wxPoint(WINDOW_WIDTH/2-launch_button->GetRect().width/2, WINDOW_HEIGHT-60));

    //Insert window resolution controls
    wxStaticText* window_res_label = new wxStaticText(panel, wxID_ANY, wxT("Window Resolution"));
    window_res_label->SetPosition(wxPoint(55, 100));

    wxTextCtrl* window_x_entry = new wxTextCtrl(panel, wxID_ANY);
    window_x_entry->SetPosition(wxPoint(50, 125));
    window_x_entry->SetSize(wxSize(60, 25));

    wxStaticText* x1 = new wxStaticText(panel, wxID_ANY, wxT("X"));
    x1->SetPosition(wxPoint(118, 125));

    wxTextCtrl* window_y_entry = new wxTextCtrl(panel, wxID_ANY);
    window_y_entry->SetPosition(wxPoint(135, 125));
    window_y_entry->SetSize(wxSize(60, 25));

    //Insert canvas resolution controls
    wxStaticText* canvas_res_label = new wxStaticText(panel, wxID_ANY, wxT("Canvas Resolution"));
    canvas_res_label->SetPosition(wxPoint(55, 160));


    wxTextCtrl* canvas_x_entry = new wxTextCtrl(panel, wxID_ANY);
    canvas_x_entry->SetPosition(wxPoint(50, 185));
    canvas_x_entry->SetSize(wxSize(60, 25));

    wxStaticText* x2 = new wxStaticText(panel, wxID_ANY, wxT("X"));
    x2->SetPosition(wxPoint(118, 185));

    wxTextCtrl* canvas_y_entry = new wxTextCtrl(panel, wxID_ANY);
    canvas_y_entry->SetPosition(wxPoint(135, 185));
    canvas_y_entry->SetSize(wxSize(60, 25));

    wxStaticText* file_picker_label = new wxStaticText(panel, wxID_ANY, wxT("Canvas File"));
    file_picker_label->SetPosition(wxPoint(55, 255));
    wxFilePickerCtrl* file_picker_ctrl = new wxFilePickerCtrl(panel, wxID_ANY);
    file_picker_ctrl->SetPosition(wxPoint(50, 270));
    file_picker_ctrl->SetSize(wxSize(150, 50));
}
void MyFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}
void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("This is a wxWidgets' Hello world sample",
        "About Hello World", wxOK | wxICON_INFORMATION);
}
void MyFrame::OnHello(wxCommandEvent& event)
{
    wxLogMessage("Hello world from wxWidgets!");
}
