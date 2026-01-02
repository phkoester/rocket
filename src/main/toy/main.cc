/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/format/std.h"
#include "rocket/log/log.h"
#include "rocket/unicode/iterator.h"

#include <libunicode/capi.h>
#include <libunicode/grapheme_segmenter.h>
#include <libunicode/utf8_grapheme_segmenter.h>

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

// Local functions ------------------------------------------------------------------------------------------

namespace {

void
toy() {
  auto& out = nio::stdout;

  out.println("rocket::unicode");
  out.println("===============");

  u32string s32 = U"a👨‍👩‍👦b";
  auto graphemes32 = unicode::graphemes(s32);
  for (int i = 0; i < graphemes32.size(); ++i) {
    const auto& gr = graphemes32[i];
    out.println("grapheme 32 #{}: {: <2}, codepoints={}, width={}", i, gr, gr.codePoints.size(), gr.width);
  }

  string s8 = "a👨‍👩‍👦b";
  auto graphemes8 = unicode::graphemes(s8);
  for (int i = 0; i < graphemes8.size(); ++i) {
    const auto& gr = graphemes32[i];
    out.println("grapheme  8 #{}: {: <2}, codepoints={}, width={}", i, gr, gr.codePoints.size(), gr.width);
  }

  out.println("unicodelib");
  out.println("==========");
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

int
main(int argc, char **argv) {
  try {
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
      ROCKET_LOG_INFO("hey {}", "there");
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
