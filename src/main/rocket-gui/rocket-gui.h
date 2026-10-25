/**
 * @file rocket-gui.h
 */

#pragma once

#include <rocket/rocket.h>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
  #include <wx/wx.h>
#endif

// Operators ------------------------------------------------------------------------------------------------

inline bool
operator<(const wxSize& lhs, const wxSize& rhs) {
  if (lhs.x != rhs.x) {
    return lhs.x < rhs.x;
  }
  return lhs.y < rhs.y;
}

namespace rocket::gui {

// Functions ------------------------------------------------------------------------------------------------

wxToolBarToolBase* addTool(
  wxToolBar* toolBar,
  i32 id,
  std::string_view label,
  std::string_view svgName,
  std::string_view shortHelp);

wxBitmapBundle loadSvg(std::string_view name, const wxSize& size);

wxBitmapBundle makeDisabled(const wxBitmapBundle& bundle);

} // namespace rocket::gui

// EOF
