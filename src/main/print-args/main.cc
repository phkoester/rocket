/*
 * main.cc
 */

#include "rocket/Process.h"
#include "rocket/format-std.h"

using namespace rocket;
using namespace std;

int
main(int argc, char **argv) {
  try {
    process.init(argc, argv, "print-args");
    for (int i = 1; i < argc; ++i) {
      string arg(argv[i]);
      vector<byte> bytes;
      bytes.reserve(arg.size());
      for_each(arg.begin(), arg.end(), [&](char c) { bytes.push_back(byte(c)); });
      auto& out = nio::stdout;
      out.println("{}: {}", i, bytes); // XXX
      out.println("{}={}=", i, arg);
    }
    process.exit(EXIT_SUCCESS);
  } catch (...) {
    terminate();
  }
}

// EOF
