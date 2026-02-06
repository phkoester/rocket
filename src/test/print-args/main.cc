/*
 * main.cc
 */

#include "rocket/Process.h"

#include <fmt/ranges.h>

using namespace rocket;
using namespace std;

i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "print-args");

  for (i32 i = 1; i < argc; ++i) {
    string arg(argv[i]);
    vector<byte> bytes;
    bytes.reserve(arg.size());
    for_each(arg.begin(), arg.end(), [&](char c) { bytes.push_back(byte(c)); });
    auto& out = nio::out;
    out.println("{}: {}", i, bytes);
    out.println("{}={}=", i, arg);
  }

  process.exit(EXIT_SUCCESS);
}

// EOF
