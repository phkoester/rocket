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

#include <wx/artprov.h>
#include <wx/taskbar.h>
#include <wx/webview.h>

using namespace rocket;
using namespace rocket::gui;
using namespace std;

// Macros ---------------------------------------------------------------------------------------------------

#define COPYRIGHT "Copyright © 2024–2026 Philip Köster"

// `GuyId` --------------------------------------------------------------------------------------------------

enum GuyId {
  ID_Hello = 1,
  ID_Back,
  ID_Forward
};

// `MyApp` decl ---------------------------------------------------------------------------------------------

struct MyApp : wxApp {
  virtual int OnExit() override;

  virtual bool OnInit() override;
};

// `MyFrame` decl -------------------------------------------------------------------------------------------

struct MyTaskBarIcon;

struct MyFrame : wxFrame {
  MyFrame();

private:

  void onAbout(wxCommandEvent& event);

  void onBack(wxCommandEvent& event);

  void onExit(wxCommandEvent& event);

  void onForward(wxCommandEvent& event);

  void onHello(wxCommandEvent& event);

  void onNavigated(wxWebViewEvent& event);

  void updateHistoryButtons();

  unique_ptr<MyTaskBarIcon> taskBarIcon_;

  wxWebView* webView_ = nullptr;
};

// `MyTaskBarIcon` decl -------------------------------------------------------------------------------------

struct MyTaskBarIcon : wxTaskBarIcon {
  explicit MyTaskBarIcon(wxFrame* frame);

  wxMenu* CreatePopupMenu() override;

private:

  void onExit(wxCommandEvent& event);

  wxFrame* frame_;
};

// `Pcl` ----------------------------------------------------------------------------------------------------

/// Parsed command line.
struct Pcl {
  optional<bool> help;
  optional<vector<string>> files;
  optional<bool> verbose;
  optional<bool> version;
};

Pcl pcl;

// Functions ------------------------------------------------------------------------------------------------

wxSize
initialFrameSize() {
  auto displaySize = wxGetDisplaySize();
  return wxSize(displaySize.x / 3, displaySize.y / 2);
}

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

MyFrame::MyFrame() : wxFrame(
  nullptr,
  wxID_ANY,
  "guy",
  wxPoint(50, 50),
  initialFrameSize()) {
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

  taskBarIcon_ = make_unique<MyTaskBarIcon>(this);

  Bind(wxEVT_MENU, &MyFrame::onAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, &MyFrame::onExit, this, wxID_EXIT);

  Bind(wxEVT_MENU, &MyFrame::onHello, this, ID_Hello);

  webView_ = wxWebView::New(this, wxID_ANY, "https://wxwidgets.org");
  if (not webView_) {
    SetStatusText("The web view backend is not available");
    return;
  }

  wxToolBar* toolBar = CreateToolBar(wxTB_DEFAULT_STYLE | wxTB_HORZ_TEXT);
  toolBar->AddTool(
    ID_Back,
    "",
    wxArtProvider::GetBitmapBundle(wxART_GO_BACK, wxART_TOOLBAR),
    "Go back to the previous page");
  toolBar->AddTool(
    ID_Forward,
    "",
    wxArtProvider::GetBitmapBundle(wxART_GO_FORWARD, wxART_TOOLBAR),
    "Go forward in the page history");
  toolBar->Realize();
  updateHistoryButtons();

  // Keyboard accelerators are delivered as menu commands; toolbar clicks stay tool events
  wxAcceleratorEntry entries[] = {
    wxAcceleratorEntry(wxACCEL_ALT, WXK_LEFT, ID_Back),
    wxAcceleratorEntry(wxACCEL_ALT, WXK_RIGHT, ID_Forward)
  };
  SetAcceleratorTable(wxAcceleratorTable(size(entries), entries));

  Bind(wxEVT_MENU, &MyFrame::onBack, this, ID_Back);
  Bind(wxEVT_MENU, &MyFrame::onForward, this, ID_Forward);

  Bind(wxEVT_TOOL, &MyFrame::onBack, this, ID_Back);
  Bind(wxEVT_TOOL, &MyFrame::onForward, this, ID_Forward);

  Bind(wxEVT_WEBVIEW_NAVIGATED, &MyFrame::onNavigated, this, webView_->GetId());
  Bind(wxEVT_WEBVIEW_LOADED, &MyFrame::onNavigated, this, webView_->GetId());

  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(webView_, 1, wxEXPAND);
  SetSizer(sizer);
}

void
MyFrame::onAbout([[maybe_unused]] wxCommandEvent& ev) {
  string msg = fmt::format(
    "This is {}.\n\n{}\n\nRocket version {}", process.name(), COPYRIGHT, ROCKET_VERSION_NAME);
  if (pcl.files) {
    msg += fmt::format("\n\nFiles from the command line: {}", *pcl.files);
  }

  wxMessageBox(msg, fmt::format("About {}", process.name()), wxICON_INFORMATION | wxOK);
}

void
MyFrame::onBack([[maybe_unused]] wxCommandEvent& ev) {
  if (webView_->CanGoBack()) {
    webView_->GoBack();
  }
}

void
MyFrame::onExit([[maybe_unused]] wxCommandEvent& ev) {
  Close(true);
}

void
MyFrame::onForward([[maybe_unused]] wxCommandEvent& ev) {
  if (webView_->CanGoForward()) {
    webView_->GoForward();
  }
}

void
MyFrame::onHello([[maybe_unused]] wxCommandEvent& ev) {
  wxLogMessage("Hello world from wxWidgets!");
}

void
MyFrame::onNavigated([[maybe_unused]] wxWebViewEvent& ev) {
  updateHistoryButtons();
}

void
MyFrame::updateHistoryButtons() {
  wxToolBar* toolBar = GetToolBar();
  if (not toolBar || not webView_) {
    return;
  }
  toolBar->EnableTool(ID_Back, webView_->CanGoBack());
  toolBar->EnableTool(ID_Forward, webView_->CanGoForward());
}

// `MyTaskBarIcon` impl -------------------------------------------------------------------------------------

MyTaskBarIcon::MyTaskBarIcon(wxFrame* frame) : frame_(frame) {
  Bind(wxEVT_MENU, &MyTaskBarIcon::onExit, this, wxID_EXIT);
  SetIcon(
    wxArtProvider::GetBitmapBundle(wxART_EXECUTABLE_FILE, wxART_OTHER, wxSize(16, 16)),
    process.name());
}

wxMenu*
MyTaskBarIcon::CreatePopupMenu() {
  wxMenu* menu = new wxMenu;
  menu->Append(wxID_EXIT, "E&xit");
  return menu;
}

void
MyTaskBarIcon::onExit([[maybe_unused]] wxCommandEvent& ev) {
  frame_->Close(true);
}

// `main` ---------------------------------------------------------------------------------------------------

wxIMPLEMENT_APP(MyApp);

// EOF
