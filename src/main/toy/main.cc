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

  auto& out = nio::stdout;

  out.println(locale("de_DE.UTF-8"), "de_DE: {:L}", 123456.78);
  out.println(locale("en_US.UTF-8"), "en_US: {:L}", 123456.78);
  optional<int> opt;
  out.println("opt: {}", opt);

  ROCKET_LOG_INFO("hi");

  Point p { 1000, 2000 };
  out.println("Point: {:L}", p);

  int n = 6000;
  // < 10: The value of n is small. Everything less than 10 is small
  // < 50: The value of n is mediuml. Everything less than 50 is small
  // >= 50: The value of n is large. Everything larger than 50 is large
  out.println(
      "A note from {}: The value of n is {}. Here goes some answer: {}",
      process.name(),
      nio::Format([&] {
    if (n < 10) {
      return nio::Format::params("{}, which is small. Everything less than {} is small", n, 10);
    } else if (n < 5000) {
      return nio::Format::params("{}, which is medium. Everything less than {} is medium", n, 5000);
    } else {
      return nio::Format::params();
    }
  }), 42);

  cout << "end of toy" << endl;
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
