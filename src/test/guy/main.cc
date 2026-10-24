/*
 * main.cc
 *
 * The `guy` test executable links to Rocket.Gui and is a playground for quick and dirty GUI experiments.
 */

#include <rocket/Process.h>
#include <rocket/version.h>
#include <rocket/cl/cl.h>
#include <rocket/nio/nio.h>

#include <rocket-gui/rocket-gui.h>

using namespace rocket;
using namespace rocket::gui;
using namespace std;

// Macros ---------------------------------------------------------------------------------------------------

#define COPYRIGHT "Copyright © 2024–2026 Philip Köster"

// `GuyId` --------------------------------------------------------------------------------------------------

enum GuyId {
  ID_Hello = 1
};

// `Pcl` ----------------------------------------------------------------------------------------------------

struct Pcl {
  optional<bool> help;
  optional<vector<string>> files;
  optional<bool> verbose;
  optional<bool> version;
};

Pcl pcl; ///< The parsed command line.

// `MyApp` decl ---------------------------------------------------------------------------------------------

struct MyApp : wxApp {
  virtual int OnExit() override;

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

int
MyApp::OnExit() {
  process.exit(EXIT_SUCCESS);
}

bool
MyApp::OnInit() {
  process.init(argc, argv, "guy");

  const cl::OptionGroup general("General control");
  const cl::CommandLineConfig config {
    .usages={ "[OPTION]... [FILE]..." },
    .version=fmt::format("{} {}\n\n{}", "Rocket", ROCKET_VERSION_NAME, COPYRIGHT)
  };
  cl::CommandLine cl({
    cl::Option::help(&general, pcl.help),
    cl::Option::verbose(&general, pcl.verbose),
    cl::Option::version(&general, pcl.version),
  }, {
    cl::Parameter::make({ .description="a file", .name="FILE" }, pcl.files)
  }, config);

  cl.parse(process.args());

  nio::out.println("This is {}", process.name());
  howdy();

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
MyFrame::OnAbout([[maybe_unused]] wxCommandEvent& ev) {
  string msg = fmt::format(
    "This is {}.\n\n{}\n\nRocket version {}", process.name(), COPYRIGHT, ROCKET_VERSION_NAME);
  if (pcl.files) {
    msg += fmt::format("\n\nFiles from the command line: {}", *pcl.files);
  }

  wxMessageBox(msg, fmt::format("About {}", process.name()), wxICON_INFORMATION | wxOK);
}

void
MyFrame::OnExit([[maybe_unused]] wxCommandEvent& ev) {
  Close(true);
}

void
MyFrame::OnHello([[maybe_unused]] wxCommandEvent& ev) {
  wxLogMessage("Hello world from wxWidgets!");
}

// `main` ---------------------------------------------------------------------------------------------------

wxIMPLEMENT_APP(MyApp);

// EOF
