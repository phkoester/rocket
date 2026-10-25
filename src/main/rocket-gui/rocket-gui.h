/**
 * @file rocket-gui.h
 */

#pragma once

#include <rocket/rocket.h>
#include <rocket/hash/hash.h>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
  #include <wx/wx.h>
#endif

namespace rocket::gui {

// Functions ------------------------------------------------------------------------------------------------

/**
 * A replacement for `wxToolBar::AddTool` that adds a proper disabled bitmap on Windows.
 *
 * @param toolBar the toolbar to add the tool to
 * @param id the ID of the tool
 * @param label the label of the tool
 * @param svgName the name of the SVG file to load
 * @param shortHelp the short help of the tool
 * @return a pointer to the new tool
 */
wxToolBarToolBase* addTool(
  wxToolBar* toolBar,
  i32 id,
  std::string_view label,
  std::string_view svgName,
  std::string_view shortHelp);

/**
 * Loads an SVG as a bitmap bundle.
 *
 * @param name the name of the SVG file to load
 * @param size the size of the bitmaps
 * @return a `wxBitmapBundle`
 */
wxBitmapBundle loadSvg(std::string_view name, const wxSize& size);

/**
 * Makes a disabled bitmap bundle from a bitmap bundle.
 *
 * This is especially useful on Windows, where the native tool bar doesn't show disabled bitmaps.
 *
 * @param bundle a bitmap bundle
 * @return a disabled bitmap bundle
 */
wxBitmapBundle makeDisabled(const wxBitmapBundle& bundle);

} // namespace rocket::gui


// `std::hash<wxSize>` --------------------------------------------------------------------------------------

/// @spec_std_hash{wxSize}
template<>
struct std::hash<wxSize> {
  /// @cond undocumented
  rocket::u64
  operator()(const wxSize& size) const {
    using namespace rocket;
    u64 ret = std::hash<i32>()(size.x);
    rocket::hash::combine(ret, std::hash<i32>()(size.y));
    return ret;
  }
  /// @endcond
};

// EOF
