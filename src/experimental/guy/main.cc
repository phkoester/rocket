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

#include <wx/aboutdlg.h>
#include <wx/artprov.h>
#include <wx/bmpbndl.h>
#include <wx/display.h>
#include <wx/taskbar.h>
#include <wx/webview.h>
#include <wx/xrc/xmlres.h>

#ifdef ROCKET_OS_WINDOWS
#include <wx/generic/aboutdlgg.h>
#include <wx/msw/wrapwin.h>
#endif

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

  /// Owner-drawn menu-bar titles on Windows. The native menu stores these pointers.
  vector<unique_ptr<wxMenuItem>> menuBarTitles_;
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

void InitXmlResource();

wxSize
initialFrameSize() {
  auto displaySize = wxGetDisplaySize();
  return wxSize(displaySize.x / 3, displaySize.y / 2);
}

wxBitmapBundle
loadLogo(wxSize size = wxSize(192, 192)) {
  return loadSvg("logo.svg", size);
}

/// A 4K mode is 3840 pixels on its long side. `GetCurrentMode()` reports the
/// hardware mode, which stays 3840 even when Windows scales the desktop.
bool
on4kMonitor(const wxWindow* window) {
#ifdef ROCKET_OS_WINDOWS
  int index = window ? wxDisplay::GetFromWindow(window) : wxNOT_FOUND;
  if (index == wxNOT_FOUND && window) {
    index = wxDisplay::GetFromPoint(window->GetPosition());
  }
  if (index == wxNOT_FOUND) {
    index = 0;
  }

  const wxDisplay display(static_cast<unsigned>(index));
  const wxVideoMode mode = display.GetCurrentMode();
  const wxSize geometry = display.GetGeometry().GetSize();
  int longest = mode.w;
  if (mode.h > longest) {
    longest = mode.h;
  }
  if (geometry.x > longest) {
    longest = geometry.x;
  }
  if (geometry.y > longest) {
    longest = geometry.y;
  }
  return longest >= 3840;
#else
  (void) window;
  return false;
#endif
}

constexpr double fourKFontScale = 1.25;

wxFont
scaledFont(const wxFont& font, double factor) {
  wxFont scaled(font);
  if (scaled.IsOk()) {
    scaled.SetFractionalPointSize(scaled.GetFractionalPointSize() * factor);
  }
  return scaled;
}

void
scaleControlFonts(wxWindow* window, double factor) {
  window->InvalidateBestSize();
  const wxFont font = scaledFont(window->GetFont(), factor);
  if (font.IsOk()) {
    window->SetFont(font);
  }
  for (wxWindow* child : window->GetChildren()) {
    scaleControlFonts(child, factor);
  }
}

#ifdef ROCKET_OS_WINDOWS

/// Menu-bar titles are not `wxMenuItem`s. Windows only accepts a custom font for
/// them when the item is owner-drawn.
struct MenuBarTitle : wxMenuItem {
  using wxMenuItem::wxMenuItem;

  bool OnMeasureItem(size_t* width, size_t* height) override {
    wxWindow* frame = GetMenu()->GetWindow();
    wxInfoDC dc(frame);
    wxFont font;
    GetFontToUse(font);
    dc.SetFont(font);
    const wxSize extent = dc.GetTextExtent(GetItemLabelText());
    *width = static_cast<size_t>(extent.x + frame->FromDIP(16));
    *height = static_cast<size_t>(extent.y + frame->FromDIP(8));
    return true;
  }

  bool OnDrawItem(wxDC& dc, const wxRect& rect, wxODAction, wxODStatus status) override {
    wxColour foreground;
    wxColour background;
    GetColourToUse(status, foreground, background);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(background));
    dc.DrawRectangle(rect);

    wxFont font;
    GetFontToUse(font);
    dc.SetFont(font);
    dc.SetTextForeground(foreground);

    wxString label;
    int accelIndex = -1;
    if ((status & wxODHidePrefix) == 0) {
      accelIndex = wxControl::FindAccelIndex(GetItemLabel(), &label);
    } else {
      label = GetItemLabelText();
    }
    dc.DrawLabel(label, rect, wxALIGN_CENTRE, accelIndex);
    return true;
  }
};

void
scaleMenuFonts(wxMenu* menu, const wxFont& font) {
  vector<wxMenuItem*> items;
  while (menu->GetMenuItemCount() > 0) {
    items.push_back(menu->Remove(menu->FindItemByPosition(0)));
  }
  for (wxMenuItem* item : items) {
    if (not item->IsSeparator()) {
      item->SetFont(font);
      if (wxMenu* subMenu = item->GetSubMenu()) {
        scaleMenuFonts(subMenu, font);
      }
    }
    menu->Append(item);
  }
}

void
scaleMenuBarTitles(wxFrame* frame, const wxFont& font, vector<unique_ptr<wxMenuItem>>& titles) {
  wxMenuBar* menuBar = frame->GetMenuBar();
  const HMENU handle = static_cast<HMENU>(menuBar->GetHMenu());
  for (size_t i = 0; i < menuBar->GetMenuCount(); ++i) {
    auto title = make_unique<MenuBarTitle>(menuBar->GetMenu(i), wxID_ANY, menuBar->GetMenuLabel(i));
    title->SetFont(font);

    MENUITEMINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_FTYPE | MIIM_DATA;
    info.fType = MFT_OWNERDRAW;
    info.dwItemData = reinterpret_cast<ULONG_PTR>(title.get());
    ::SetMenuItemInfo(handle, static_cast<UINT>(i), TRUE, &info);
    titles.push_back(std::move(title));
  }
  ::DrawMenuBar(static_cast<HWND>(frame->GetHWND()));
}

#endif

void
enlargeChromeFonts(wxFrame* frame, vector<unique_ptr<wxMenuItem>>& menuBarTitles) {
#ifdef ROCKET_OS_WINDOWS
  if (not on4kMonitor(frame)) {
    return;
  }

  const wxFont font = scaledFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT), fourKFontScale);
  wxMenuBar* menuBar = frame->GetMenuBar();
  for (size_t i = 0; i < menuBar->GetMenuCount(); ++i) {
    scaleMenuFonts(menuBar->GetMenu(i), font);
  }
  scaleMenuBarTitles(frame, font, menuBarTitles);

  wxStatusBar* statusBar = frame->GetStatusBar();
  statusBar->SetFont(scaledFont(statusBar->GetFont(), fourKFontScale));
  statusBar->InvalidateBestSize();
#else
  (void) frame;
  (void) menuBarTitles;
#endif
}

void
showAbout(wxWindow* parent, const wxAboutDialogInfo& info) {
#ifdef ROCKET_OS_WINDOWS
  if (on4kMonitor(parent)) {
    wxGenericAboutDialog dialog;
    if (dialog.Create(info, parent)) {
      scaleControlFonts(&dialog, fourKFontScale);
      dialog.Fit();
      dialog.CentreOnParent();
      dialog.ShowModal();
      return;
    }
  }
#endif
  wxAboutBox(info, parent);
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

  wxXmlResource::Get()->InitAllHandlers();
  InitXmlResource();

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
  const wxBitmapBundle logo = loadLogo();
  if (logo.IsOk()) {
    wxIconBundle icons;
    for (const int size : { 16, 32, 48, 64, 128, 256 }) {
      icons.AddIcon(logo.GetIcon(wxSize(size, size)));
    }
    SetIcons(icons);
  }

  wxMenu* menuFile = new wxMenu;
  menuFile->Append(ID_Hello, "&Hello ...", "Say hello");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT, "E&xit", "Exit the application");

  wxMenu* menuView = new wxMenu;
  menuView->Append(ID_Back, "Go &Back\tAlt+Left", "Go back");
  menuView->Append(ID_Forward, "Go &Forward\tAlt+Right", "Go forward");

  wxMenu* menuHelp = new wxMenu;
  menuHelp->Append(wxID_ABOUT, "&About", "About the application");

  wxMenuBar* menuBar = new wxMenuBar;
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuView, "&View");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  CreateStatusBar();
  SetStatusText("Welcome to wxWidgets!");
  enlargeChromeFonts(this, menuBarTitles_);

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
  addTool(toolBar, ID_Back, "", "back.svg", "Go Back\tAlt+Left");
  addTool(toolBar, ID_Forward, "", "forward.svg", "Go Forward\tAlt+Right");
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
  string description = fmt::format("This is {}.", process.name());
  if (pcl.files) {
    description += fmt::format("\n\nFiles from the command line: {}", *pcl.files);
  }

  wxAboutDialogInfo info;
  info.SetCopyright(COPYRIGHT);
  info.SetDescription(description);
  info.SetName(process.name());
  info.SetVersion(fmt::format("Version {}", ROCKET_VERSION_NAME));
  info.SetWebSite("https://github.com/phkoester/rocket");

  const wxBitmapBundle logo = loadLogo();
  if (logo.IsOk()) {
    info.SetIcon(logo);
  }

  showAbout(this, info);
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

  wxBitmapBundle icon = loadLogo(wxSize(24, 24));
  if (not icon.IsOk()) {
    icon = wxArtProvider::GetBitmapBundle(wxART_EXECUTABLE_FILE, wxART_OTHER, wxSize(16, 16));
  }
  SetIcon(icon, process.name());
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
