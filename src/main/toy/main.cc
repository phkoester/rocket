/*
 * main.cc
 */

#include "rocket/codec-rocket-decl.h"
#include "rocket/codec-std-decl.h"
#include "rocket/codec-rocket.h"
#include "rocket/codec-std.h"

#include "rocket/Process.h"
#include "rocket/cl.h"
#include "rocket/log.h"

#include <fmt/ranges.h>
#include <fmt/std.h>

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

// `Color` --------------------------------------------------------------------------------------------------

enum class Color {
  red,
  green,
  blue,
};

template<>
struct fmt::formatter<Color> : formatter<string_view> {
  template<typename FormatContext>
  constexpr auto format(Color v, FormatContext& ctx) const {
    string_view name;
    switch (v) {
    case Color::red:   name = "red"; break;
    case Color::green: name = "green"; break;
    case Color::blue:  name = "blue"; break;
    default: ROCKET_CHECK(v, false, "Invalid `{}`: {}", ::rocket::Type::of<Color>().name(), static_cast<int>(v));
    }
    return formatter<string_view>::format(name, ctx);
  }
};

// `Point` --------------------------------------------------------------------------------------------------

struct Point {
  int x, y;
};

template<>
struct fmt::formatter<Point> {
  template<typename FormatContext>
  constexpr auto format(const Point& v, FormatContext& ctx) const {
    string pointFormat = fmt::format("({{:{0}}}, {{:{0}}})", intFormat_);
    return format_to(ctx.out(), fmt::runtime(pointFormat), v.x, v.y);
  }

  constexpr auto parse(format_parse_context& ctx) {
    auto end = formatter<int>().parse(ctx);
    intFormat_ = string_view(ctx.begin(), end - ctx.begin());
    return end;
  }

private:

  string_view intFormat_;
};

// Local functions ------------------------------------------------------------------------------------------

namespace {

void
toy() {
  ROCKET_LOG(toy);

  ROCKET_LOG_INFO("hi");
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

int
main(int argc, char **argv) {
  try {
    ROCKET_ERROR("Test error");
    ROCKET_PROCESS_ERROR("Test process error");

    process.init(argc, argv, "toy");

    cl::CommandLine cl;
    vector<string> args;
    try {
      args = cl.parse(process.args());
    } catch (const exception& ex) {
      cl.handleException(ex, nio::stderr);
    }

    {
      ROCKET_LOG(toy);
      auto& out = nio::stdout;
      out.println("This is {}", process.name());
      out.println("args: {}", args);
      toy();
    }

    process.exit(EXIT_SUCCESS);
  } catch (...) {
    terminate();
  }
}

// EOF
