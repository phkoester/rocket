// main.cc

#include <rocket/Process.h>
#include <rocket/filesystem/filesystem.h>

#include <fmt/format.h>
#include <fmt/std.h>

#include <algorithm>
#include <ranges>
#include <string>
#include <utility> // `std::pair`
#include <vector>

using namespace rocket;
using namespace std;

namespace {

// Local types ----------------------------------------------------------------------------------------------

using Row = pair<string, string>;

// Local functions ------------------------------------------------------------------------------------------

void
printTable(const vector<Row>& rows) {
  const u64 col0w = rows.empty() ?
    0 :
    ranges::max(rows | views::transform([](const Row& row) { return row.first.size(); }));
  for (const auto& [col0, col1] : rows) {
    nio::out.println("{: <{}} | {}", col0, col0w, col1);
  }
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process().init(argc, argv, "print-env");

  vector<Row> rows;
  rows.emplace_back("`filesystem::systemTempDir`", fmt::format("{}", rocket::filesystem::systemTempDir()));
  rows.emplace_back("`filesystem::userTempDir`", fmt::format("{}", rocket::filesystem::userTempDir()));

  printTable(rows);

  process().exit(EXIT_SUCCESS);
}

// EOF
