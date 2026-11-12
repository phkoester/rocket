/*
 * archive.cc
 */

#include "rocket/archive/archive.h"

#include "rocket/assert.h"
#include "rocket/scope.h"

#include <array>
#include <fstream>
#include <memory>
#include <system_error>

#ifndef LIBARCHIVE_STATIC
  #define LIBARCHIVE_STATIC
#endif
#include <archive.h>
#include <archive_entry.h>

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

const char*
errorString(struct archive* a) {
  const char* ret = archive_error_string(a);
  return ret ? ret : "Unknown error";
}

/// Throws #rocket::InvalidState if @p status signals an error.
void
checkStatus(struct archive* a, int status, string_view what, const fs::path& path) {
  if (status < ARCHIVE_WARN) {
    ROCKET_FAIL("{} `{}` failed: {}", what, path.string(), errorString(a));
  }
}

ArchiveEntryPtr
newEntry() {
  ArchiveEntryPtr ret(archive_entry_new());
  if (not ret) {
    ROCKET_FAIL("Cannot allocate archive entry");
  }
  return ret;
}

ArchiveWritePtr
openWriter(const fs::path& dest, bool tar) {
  ArchiveWritePtr ret(archive_write_new());
  if (not ret) {
    ROCKET_FAIL("Cannot allocate archive writer");
  }
  auto* a = ret.get();
  checkStatus(a, tar ? archive_write_set_format_pax_restricted(a) : archive_write_set_format_raw(a),
    "Setting the archive format for", dest);
  checkStatus(a, archive_write_add_filter_gzip(a), "Setting the gzip filter for", dest);
#ifdef ROCKET_OS_WINDOWS
  checkStatus(a, archive_write_open_filename_w(a, dest.c_str()), "Opening", dest);
#else
  checkStatus(a, archive_write_open_filename(a, dest.c_str()), "Opening", dest);
#endif
  return ret;
}

/// Writes the contents of the file @p path as data of the current entry.
void
writeData(struct archive* a, const fs::path& path, const fs::path& dest) {
  ifstream in(path, ios::binary);
  if (not in) {
    ROCKET_FAIL("Cannot open file `{}` for reading", path.string());
  }
  array<char, 64 * 1024> buf; // NOLINT(*-member-init)
  while (in) {
    in.read(buf.data(), static_cast<streamsize>(buf.size()));
    const auto n = in.gcount();
    if (n > 0 and archive_write_data(a, buf.data(), static_cast<size_t>(n)) < 0) {
      ROCKET_FAIL("Writing data of `{}` to `{}` failed: {}", path.string(), dest.string(), errorString(a));
    }
  }
  if (in.bad()) {
    ROCKET_FAIL("Cannot read file `{}`", path.string());
  }
}

void
gzipFile(const fs::path& srce, const fs::path& dest) {
  auto writer = openWriter(dest, false);
  auto* a = writer.get();

  // The raw format requires exactly one regular-file entry; its metadata is not stored
  auto entry = newEntry();
  archive_entry_set_filetype(entry.get(), AE_IFREG);
  checkStatus(a, archive_write_header(a, entry.get()), "Writing a header to", dest);
  writeData(a, srce, dest);

  checkStatus(a, archive_write_close(a), "Closing", dest);
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
  if (archive_read_disk_entry_from_file(disk, e, -1, nullptr) < ARCHIVE_WARN) {
    ROCKET_FAIL("Cannot read metadata of `{}`: {}", path.string(), errorString(disk));
  }
  checkStatus(a, archive_write_header(a, e), "Writing a header to", dest);
  if (archive_entry_filetype(e) == AE_IFREG and archive_entry_size(e) > 0) {
    writeData(a, path, dest);
  }
}

void
gzipDir(const fs::path& srce, const fs::path& dest) {
  ArchiveReadPtr diskPtr(archive_read_disk_new());
  if (not diskPtr) {
    ROCKET_FAIL("Cannot allocate disk reader");
  }
  auto* disk = diskPtr.get();
  // Store symbolic links as links instead of following them
  checkStatus(disk, archive_read_disk_set_symlink_physical(disk), "Configuring the disk reader for", srce);
  checkStatus(disk, archive_read_disk_set_standard_lookup(disk), "Configuring the disk reader for", srce);

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
    ROCKET_FAIL("Cannot traverse directory `{}`: {}", srce.string(), ec.message());
  }

  checkStatus(a, archive_write_close(a), "Closing", dest);
}

} // namespace

namespace rocket::archive {

// Functions ------------------------------------------------------------------------------------------------

void
gzip(const fs::path& srce, const fs::path& dest) {
  ROCKET_CHECK(srce, not srce.empty(), "Source path is empty");
  ROCKET_CHECK(dest, not dest.empty(), "Destination path is empty");

  error_code ec;
  const auto status = fs::status(srce, ec);
  ROCKET_CHECK(srce, not ec, "Cannot access `{}`: {}", srce.string(), ec.message());
  const bool isFile = fs::is_regular_file(status);
  const bool isDir = fs::is_directory(status);
  ROCKET_CHECK(srce, isFile or isDir, "Path `{}` is neither a regular file nor a directory", srce.string());
  ROCKET_CHECK(dest, not fs::is_directory(dest), "Destination `{}` is a directory", dest.string());

  // Normalize paths so that comparisons and `parent_path()` are reliable (e.g., for `dir/` or `.`)
  const fs::path absSrce = fs::weakly_canonical(fs::absolute(srce));
  const fs::path absDest = fs::weakly_canonical(fs::absolute(dest));
  ROCKET_CHECK(dest, absDest != absSrce, "Source and destination are the same path `{}`", srce.string());
  if (isDir) {
    const auto rel = absDest.lexically_relative(absSrce);
    ROCKET_CHECK(dest, rel.empty() or *rel.begin() == "..",
      "Destination `{}` is located inside `{}`", dest.string(), srce.string());
  }

  // Remove a partially written destination on failure
  scope_fail cleanup([&]() noexcept {
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
