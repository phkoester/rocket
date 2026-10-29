/*
 * rocket-gui.cc
 */

#include "rocket-gui.h"

#include <rocket/assert.h>

#include <wx/filesys.h>

#include <unordered_map>

using namespace std;

namespace {

// `DisabledBitmapBundle` -----------------------------------------------------------------------------------

struct DisabledBitmapBundle : wxBitmapBundleImpl {
  explicit DisabledBitmapBundle(const wxBitmapBundle& source) : source_(source) {}

  wxBitmap
  GetBitmap(const wxSize& size) override {
    auto it = cache_.find(size);
    if (it != cache_.end()) {
      return it->second;
    }

    const wxBitmap bitmap = makeDisabledBitmap(source_.GetBitmap(size));
    cache_.insert({ size, bitmap });
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

  unordered_map<wxSize, wxBitmap> cache_;
  wxBitmapBundle source_;

  wxBitmap
  makeDisabledBitmap(const wxBitmap& bitmap) {
    ROCKET_CHECK(bitmap, bitmap.IsOk());
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
  string path = "memory:XRC_resource/resource.cc$"s.append(name);
  const unique_ptr<wxFSFile> file { fs.OpenFile(path) };
  ROCKET_EXPECT(file != nullptr, "Cannot open file at path `{}`", path);

  wxInputStream& stream = *file->GetStream();
  const auto len = static_cast<size_t>(stream.GetLength());
  wxCharBuffer buf(len);
  ROCKET_EXPECT(stream.ReadAll(buf.data(), len), "Cannot read stream");
  auto ret = wxBitmapBundle::FromSVG(buf.data(), size);
  ROCKET_EXPECT(ret.IsOk(), "Cannot make bitmap bundle from SVG");
  return ret;
}

/// `wxBitmap::MakeDisabled()` only greyscales, so a black stroke stays black.
wxBitmapBundle
makeDisabled(const wxBitmapBundle& bundle) {
  ROCKET_CHECK(bundle, bundle.IsOk());
  auto ret = wxBitmapBundle::FromImpl(new DisabledBitmapBundle(bundle));
  ROCKET_EXPECT(ret.IsOk(), "Cannot make disabled bitmap bundle");
  return ret;
}

} // namespace rocket::gui

// EOF
