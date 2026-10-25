/*
 * rocket-gui.cc
 */

#include "rocket-gui.h"

#include <wx/filesys.h>

using namespace std;

namespace {

// `DisabledBitmapBundle` -----------------------------------------------------------------------------------

struct DisabledBitmapBundle : wxBitmapBundleImpl {
  explicit DisabledBitmapBundle(const wxBitmapBundle& source) : source_(source) {}

  wxBitmap
  GetBitmap(const wxSize& size) override {
    for (const Entry& entry : cache_) {
      if (entry.size == size) {
        return entry.bitmap;
      }
    }
    const wxBitmap bitmap = makeDisabledBitmap(source_.GetBitmap(size));
    cache_.push_back({ size, bitmap });
    return bitmap;
  }

  wxSize
  GetDefaultSize() const override {
    return source_.GetDefaultSize();
  }

  wxSize
  GetPreferredBitmapSizeAtScale(double scale) const override {
    return source_.GetPreferredBitmapSizeAtScale(scale);
  }

private:

  struct Entry {
    wxSize size;
    wxBitmap bitmap;
  };

  vector<Entry> cache_; // XXX map!
  wxBitmapBundle source_;

  wxBitmap
  makeDisabledBitmap(const wxBitmap& bitmap) {
    if (not bitmap.IsOk()) {
      return {};
    }
    const double scale = bitmap.GetScaleFactor();
    const wxImage image = bitmap.ConvertToImage().ConvertToGreyscale().ConvertToDisabled();
    return wxBitmap(image, -1, scale);
  }
};

} // namespace

namespace rocket::gui {

wxToolBarToolBase*
addTool(wxToolBar* toolBar, i32 id, string_view label, string_view svgName, string_view shortHelp) {
  wxBitmapBundle bundle = loadSvg(svgName, wxSize(24, 24));
  auto* ret = toolBar->AddTool(id, string(label), bundle, string(shortHelp));
#ifdef ROCKET_OS_WINDOWS
  ret->SetDisabledBitmap(makeDisabled(bundle));
#endif
  return ret;
}

/// SVG embedded by `wxrc` from `resource.cc`.
wxBitmapBundle
loadSvg(string_view name, const wxSize& size) {
  wxFileSystem fs;
  const unique_ptr<wxFSFile> file { fs.OpenFile("memory:XRC_resource/resource.cc$"s.append(name)) };
  if (not file) {
    return {};
  }

  wxInputStream& stream = *file->GetStream();
  const auto len = static_cast<size_t>(stream.GetLength());
  wxCharBuffer buf(len);
  if (not stream.ReadAll(buf.data(), len)) {
    return {};
  }
  return wxBitmapBundle::FromSVG(buf.data(), size);
}

/// `wxBitmap::MakeDisabled()` only greyscales, so a black stroke stays black.
wxBitmapBundle
makeDisabled(const wxBitmapBundle& bundle) {
  if (not bundle.IsOk()) {
    return {};
  }
  return wxBitmapBundle::FromImpl(new DisabledBitmapBundle(bundle));
}

} // namespace rocket::gui

// EOF
