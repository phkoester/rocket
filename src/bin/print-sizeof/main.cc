// main.cc

#include <rocket/Process.h>
#include <rocket/TypeId-format.h>
#include <rocket/str/Noun.h>

#include <fmt/format.h>

#include <algorithm>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace rocket;
using namespace std;

namespace {

// Local types ----------------------------------------------------------------------------------------------

using Row = pair<string, string>;

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
void addSize(vector<Row>& rows) {
  rows.emplace_back(fmt::format("{}", TypeId::of<T>()),
    fmt::format("{:>2} {}", sizeof(T), str::noun::byte(sizeof(T))));
}

void printTable(const vector<Row>& rows) {
  const size_t leftColumnWidth = rows.empty() ?
    0 :
    ranges::max(rows | views::transform([](const Row& row) { return row.first.size(); }));
  for (const auto& [left, right] : rows) {
    nio::out.println("{: <{}} | {}", left, leftColumnWidth, right);
  }
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process().init(argc, argv, "print-sizeof");

  vector<Row> rows;
  addSize<bool>(rows);
  addSize<wchar_t>(rows);
  addSize<short>(rows);
  addSize<int>(rows);
  addSize<long>(rows);
  addSize<long long>(rows);
  addSize<i128>(rows);
  addSize<float>(rows);
  addSize<double>(rows);
  addSize<long double>(rows);
  addSize<void*>(rows);

  printTable(rows);

  process().exit(EXIT_SUCCESS);
}

// EOF
