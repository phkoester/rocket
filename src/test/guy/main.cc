/*
 * main.cc
 *
 * The `guy` test executable links to Rocket.Gui and is a playground for quick and dirty GUI experiments.
 */

#include <rocket/Process.h>
#include <rocket/nio/nio.h>
#include <rocket-gui/rocket-gui.h>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
  #include <wx/wx.h>
#endif

using namespace rocket;
using namespace rocket::gui;

// `Command` ------------------------------------------------------------------------------------------------

enum Command {
  ID_Hello = 1
};

// `MyApp` decl ---------------------------------------------------------------------------------------------

struct MyApp : wxApp {
  virtual bool OnInit() override;
};

// `MyFrame` decl -------------------------------------------------------------------------------------------

struct MyFrame : wxFrame
{
  MyFrame();

private:

  void OnAbout(wxCommandEvent& event);

  void OnExit(wxCommandEvent& event);

  void OnHello(wxCommandEvent& event);
};

// `MyApp` impl ---------------------------------------------------------------------------------------------

bool
MyApp::OnInit() {
  MyFrame* frame = new MyFrame();
  frame->Show(true);
  return true;
}

// `MyFrame` impl -------------------------------------------------------------------------------------------

MyFrame::MyFrame() : wxFrame(nullptr, wxID_ANY, "Hello World") {
  wxMenu* menuFile = new wxMenu;
  menuFile->Append(
    ID_Hello,
    "&Hello...\tCtrl-H",
    "Help string shown in status bar for this menu item");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu* menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar* menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  CreateStatusBar();
  SetStatusText("Welcome to wxWidgets!");

  Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);

  Bind(wxEVT_MENU, &MyFrame::OnHello, this, ID_Hello);
}

void
MyFrame::OnExit([[maybe_unused]] wxCommandEvent& ev) {
  Close(true);
}

void
MyFrame::OnAbout([[maybe_unused]] wxCommandEvent& ev) {
  wxMessageBox("This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION);
}

void
MyFrame::OnHello([[maybe_unused]] wxCommandEvent& ev) {
  wxLogMessage("Hello world from wxWidgets!");
}

// `main` ---------------------------------------------------------------------------------------------------

wxIMPLEMENT_APP(MyApp);

#if 0
i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "guyy");
  nio::out.println("This is {}", process.name());
  howdy();
  process.exit(EXIT_SUCCESS);
}
#endif

// EOF
