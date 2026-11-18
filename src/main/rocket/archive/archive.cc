// archive.cc

#include "rocket/SystemError.h"
#include "rocket/archive/archive.h"

#include "rocket/assert.h"
#include "rocket/scope.h"

#include <fmt/std.h>

#include <archive.h>
#include <archive_entry.h>

#include <array>
#include <cerrno>
#include <fstream>
#include <memory>
#include <new>

using namespace rocket;
using namespace std;

namespace fs = std::filesystem;

namespace {

// Local types ----------------------------------------------------------------------------------------------

struct ArchiveWriteDeleter {
  void operator()(struct archive* a) const noexcept { archive_write_free(a); }
};

struct ArchiveReadDeleter {
  void operator()(struct archive* a) const noexcept { archive_read_free(a); }
};

struct ArchiveEntryDeleter {
  void operator()(archive_entry* e) const noexcept { archive_entry_free(e); }
};

using ArchiveWritePtr = unique_ptr<struct archive, ArchiveWriteDeleter>;
using ArchiveReadPtr = unique_ptr<struct archive, ArchiveReadDeleter>;
using ArchiveEntryPtr = unique_ptr<archive_entry, ArchiveEntryDeleter>;

// Local functions ------------------------------------------------------------------------------------------

/// Throws #rocket::SystemError for the last error of @p a.
///
/// As far as possible, libarchive reports `errno` values. For an error without one, it reports the negative
/// `ARCHIVE_ERRNO_MISC`. The error code is @p fallback then, and libarchive’s error string is added to the
/// message.
[[noreturn]] void
throwError(struct archive* a, errc fallback, string_view what, const fs::path& path) {
  const i32 err = archive_errno(a);
  if (err > 0) {
    throw SystemError(error_code(err, generic_category()), fmt::format("{} `{}` failed", what, path));
  }
  const char* str = archive_error_string(a);
  throw SystemError(errorCode(fallback), str != nullptr ?
    fmt::format("{} `{}` failed: {}", what, path, str) :
    fmt::format("{} `{}` failed", what, path));
}

/// Throws #rocket::SystemError if @p status signals an error.
void
checkStatus(struct archive* a, i32 status, errc fallback, string_view what, const fs::path& path) {
  if (status < ARCHIVE_WARN) {
    throwError(a, fallback, what, path);
  }
}

ArchiveEntryPtr
newEntry() {
  ArchiveEntryPtr ret(archive_entry_new());
  if (not ret) {
    throw bad_alloc();
  }
  return ret;
}

ArchiveWritePtr
openWriter(const fs::path& dest, bool tar) {
  ArchiveWritePtr ret(archive_write_new());
  if (not ret) {
    throw bad_alloc();
  }
  auto* a = ret.get();
  checkStatus(a, tar ? archive_write_set_format_pax_restricted(a) : archive_write_set_format_raw(a),
    errc::not_supported, "setting the archive format for", dest);
  checkStatus(a, archive_write_add_filter_gzip(a), errc::not_supported, "setting the gzip filter for", dest);
#ifdef ROCKET_OS_WINDOWS
  checkStatus(a, archive_write_open_filename_w(a, dest.c_str()), errc::io_error, "opening", dest);
#else
  checkStatus(a, archive_write_open_filename(a, dest.c_str()), errc::io_error, "opening", dest);
#endif
  return ret;
}

/// Writes the contents of the file @p path as data of the current entry.
void
writeData(struct archive* a, const fs::path& path, const fs::path& dest) {
  errno = 0;
  ifstream in(path, ios::binary);
  if (not in) {
    throw SystemError(errorCode(in), fmt::format("cannot open file `{}` for reading", path));
  }
  array<char, 64 * 1024> buf; // NOLINT(*-member-init)
  while (in) {
    errno = 0;
    in.read(buf.data(), static_cast<streamsize>(buf.size()));
    if (in.bad()) {
      throw SystemError(errorCode(in), fmt::format("cannot read file `{}`", path));
    }
    const auto n = in.gcount();
    if (n > 0 and archive_write_data(a, buf.data(), static_cast<size_t>(n)) < 0) {
      throwError(a, errc::io_error, fmt::format("writing data of `{}` to", path), dest);
    }
  }
}

void
gzipFile(const fs::path& srce, const fs::path& dest) {
  auto writer = openWriter(dest, false);
  auto* a = writer.get();

  // The raw format requires exactly one regular-file entry; its metadata is not stored
  auto entry = newEntry();
  archive_entry_set_filetype(entry.get(), AE_IFREG);
  checkStatus(a, archive_write_header(a, entry.get()), errc::io_error, "writing a header to", dest);
  writeData(a, srce, dest);

  checkStatus(a, archive_write_close(a), errc::io_error, "closing", dest);
}

/// Adds the file system object at @p path to the tar archive as @p name, filling metadata from disk.
void
addTarEntry(struct archive* a, struct archive* disk, const fs::path& path, const fs::path& name,
  const fs::path& dest) {
  auto entry = newEntry();
  auto* e = entry.get();
#ifdef ROCKET_OS_WINDOWS
  archive_entry_copy_sourcepath_w(e, path.c_str());
  archive_entry_copy_pathname_w(e, name.generic_wstring().c_str());
#else
  archive_entry_copy_sourcepath(e, path.c_str());
  archive_entry_copy_pathname(e, name.generic_string().c_str());
#endif
  checkStatus(disk, archive_read_disk_entry_from_file(disk, e, -1, nullptr), errc::io_error,
    "reading metadata of", path);
  checkStatus(a, archive_write_header(a, e), errc::io_error, "writing a header to", dest);
  if (archive_entry_filetype(e) == AE_IFREG and archive_entry_size(e) > 0) {
    writeData(a, path, dest);
  }
}

void
gzipDir(const fs::path& srce, const fs::path& dest) {
  const ArchiveReadPtr diskPtr(archive_read_disk_new());
  if (not diskPtr) {
    throw bad_alloc();
  }
  auto* disk = diskPtr.get();
  // Store symbolic links as links instead of following them
  checkStatus(disk, archive_read_disk_set_symlink_physical(disk), errc::not_supported,
    "configuring the disk reader for", srce);
  checkStatus(disk, archive_read_disk_set_standard_lookup(disk), errc::not_supported,
    "configuring the disk reader for", srce);

  auto writer = openWriter(dest, true);
  auto* a = writer.get();

  // Entry names are relative to the parent, so the directory itself is the top-level entry
  const fs::path base = srce.parent_path();
  addTarEntry(a, disk, srce, srce.filename(), dest);

  error_code ec;
  fs::recursive_directory_iterator it(srce, fs::directory_options::none, ec);
  const fs::recursive_directory_iterator end;
  for (; not ec and it != end; it.increment(ec)) {
    const fs::path& path = it->path();
    addTarEntry(a, disk, path, path.lexically_relative(base), dest);
  }
  if (ec) {
    throw SystemError(ec, fmt::format("cannot traverse directory `{}`", srce));
  }

  checkStatus(a, archive_write_close(a), errc::io_error, "closing", dest);
}

} // namespace

namespace rocket::archive {

// Functions ------------------------------------------------------------------------------------------------

void
gzip(const fs::path& srce, const fs::path& dest) {
  ROCKET_CHECK(srce, not srce.empty(), "source path is empty");
  ROCKET_CHECK(dest, not dest.empty(), "destination path is empty");
  ROCKET_CHECK(dest, not fs::is_directory(dest), "destination `{}` is a directory", dest);

  error_code ec;
  const auto status = fs::status(srce, ec);
  if (ec) {
    throw SystemError(ec, fmt::format("cannot access `{}`", srce));
  }
  const bool isFile = fs::is_regular_file(status);
  const bool isDir = fs::is_directory(status);
  if (status.type() == fs::file_type::not_found) {
    throw SystemError(errorCode(errc::no_such_file_or_directory), fmt::format("cannot access `{}`", srce));
  }
  ROCKET_CHECK(srce, isFile or isDir, "path `{}` is neither a regular file nor a directory", srce);

  // Normalize paths so that comparisons and `parent_path()` are reliable (e.g., for `dir/` or `.`)
  const fs::path absSrce = fs::weakly_canonical(fs::absolute(srce));
  const fs::path absDest = fs::weakly_canonical(fs::absolute(dest));
  ROCKET_CHECK(dest, absDest != absSrce, "source and destination are the same path `{}`", srce);
  if (isDir) {
    const auto rel = absDest.lexically_relative(absSrce);
    ROCKET_CHECK(dest, rel.empty() or *rel.begin() == "..",
      "destination `{}` is located inside `{}`", dest, srce);
  }

  // Remove a partially written destination on failure
  ROCKET_SCOPE_FAIL([&]() noexcept {
    error_code ignored;
    fs::remove(absDest, ignored);
  });

  if (isFile) {
    gzipFile(absSrce, absDest);
  } else {
    gzipDir(absSrce, absDest);
  }
}

} // namespace rocket::archive

// EOF
