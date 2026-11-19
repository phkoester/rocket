// main.cc

#include <rocket/Process.h>
#include <rocket/TypeId-format.h>
#include <rocket/str/Noun.h>

#include <fmt/format.h>

#include <algorithm>
#include <ranges>
#include <string>
#include <tuple>
#include <vector>

using namespace rocket;
using namespace std;

namespace {

// Local types ----------------------------------------------------------------------------------------------

using Row = tuple<string, string, string>;

// Local functions ------------------------------------------------------------------------------------------

template<typename T>
void
addRow(vector<Row>& rows, string_view name) {
  rows.emplace_back(
    fmt::format("{}", name),
    fmt::format("{}", TypeId::of<T>()),
    fmt::format("{:>2} {}", sizeof(T), str::noun::byte(sizeof(T))));
}

void
printTable(const vector<Row>& rows) {
  const u64 col0w = rows.empty() ?
    0 :
    ranges::max(rows | views::transform([](const Row& row) { return std::get<0>(row).size(); }));
  const u64 col1w = rows.empty() ?
    0 :
    ranges::max(rows | views::transform([](const Row& row) { return std::get<1>(row).size(); }));
  for (const auto& [col0, col1, col2] : rows) {
    nio::out.println("{: <{}} | {: <{}} | {}", col0, col0w, col1, col1w, col2);
  }
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process().init(argc, argv, "print-sizeof");

  vector<Row> rows;
  addRow<bool>(rows, "bool");
  addRow<wchar_t>(rows, "wchar_t");
  addRow<short>(rows, "short");
  addRow<int>(rows, "int");
  addRow<long>(rows, "long");
  addRow<long long>(rows, "long long");
  addRow<i128>(rows, "i128");
  addRow<float>(rows, "float");
  addRow<double>(rows, "double");
  addRow<long double>(rows, "long double");
  addRow<void*>(rows, "void*");

  printTable(rows);

  process().exit(EXIT_SUCCESS);
}

// EOF
