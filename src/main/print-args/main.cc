/*
 * main.cc
 */

#include "rocket/codec-std-decl.h"
#include "rocket/codec-std.h"

#include "rocket/S.h"
#include "rocket/Process.h"

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
      cout << i << ':' << (S << bytes) << '\n';
      cout << i << '=' << arg << "=\n";
    }
    process.exit(EXIT_SUCCESS);
  } catch (...) {
    terminate();
  }
}

// EOF
