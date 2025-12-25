/*
 * main.cc
 */

#include "rocket/codec-rocket-decl.h"
#include "rocket/codec-std-decl.h"
#include "rocket/codec-rocket.h"
#include "rocket/codec-std.h"

#include "rocket/Process.h"
#include "rocket/S.h"
#include "rocket/cl.h"
#include "rocket/log.h"
#include "rocket/nio.h"

#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/facilities/check_empty.hpp>
#include <boost/preprocessor/if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/tuple/pop_front.hpp>
#include <boost/preprocessor/tuple/rem.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/tuple/enum.hpp>

#include <fmt/ranges.h>

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

// Local functions ------------------------------------------------------------------------------------------

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

namespace {

template<typename... T>
void myAssertFailedImpl(
    const std::source_location& sl,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  cout << "myAssertFailed, expr='" << expr << "'" << endl;
  fmt::string_view sv = fmt;
  if (sv.size() > 0) {
    nio::stdout().vprintln(process.codeLocale(), fmt, fmt::make_format_args(args...));
  }
}

template<typename... T>
void myAssertFailed(
    const std::source_location& sl,
    const char* expr,
    fmt::format_string<T...> fmt = "",
    T&&... args) {
  myAssertFailedImpl(sl, expr, fmt, std::forward<T>(args)...);
}

#define MY_ASSERT(expr, ...) \
    if (not (expr)) { \
      myAssertFailed( \
        ::std::source_location::current(), \
         BOOST_PP_STRINGIZE(expr) \
         BOOST_PP_COMMA_IF(BOOST_PP_NOT(BOOST_PP_CHECK_EMPTY(BOOST_PP_TUPLE_ELEM(0, (__VA_ARGS__))))) \
         __VA_ARGS__ \
      ); \
    }

void
toy() {
  ROCKET_LOG(toy);

  MY_ASSERT(false);
  MY_ASSERT(false, "oops {}", 42);

  ROCKET_ASSERT(false, "oops {}", 42);

  auto sink = nio::stdout();

  sink.println(locale("de_DE.UTF-8"), "de_DE: {:L}", 123456.78);
  sink.println(locale("en_US.UTF-8"), "en_US: {:L}", 123456.78);

  ROCKET_LOG_INFO("hi");

  Point p { 1000, 2000 };
  sink.println("Point: {:L}", p);
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
      cl.handleException(ex);
    }

    {
      ROCKET_LOG(toy);
      cout << "This is " << process.name() << '\n';
      cout << "args: " << (S << args) << "\n";
      toy();
    }

    process.exit(EXIT_SUCCESS);
  } catch (...) {
    terminate();
  }
}

// EOF
