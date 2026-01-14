/*
 * main.cc
 */

#include "rocket/Cow.h"
#include "rocket/Process.h"
#include "rocket/cl/cl.h"
#include "rocket/log/log.h"

#include <iostream>
#include <vector>

using namespace rocket;
using namespace std;

ROCKET_LOG_DEFINE(toy);

// Local functions ------------------------------------------------------------------------------------------

namespace {

struct A {
  int i = 3;
  A() { cout << "A(), i=" << i << endl; }
  ~A() { cout << "~A()" << endl; }
};

struct B {
  B() { cout << "B()" << endl; }
  ~B() { cout << "~B()" << endl; }
};

void
toy() {
  ROCKET_LOG(toy);

  ROCKET_LOG_TRACE("Hey {}", "there");

  string s = "Hello there";

  Cow<string_view, string> cow(s);
  cout << "cow=" << cow.get() << endl;
  cow = "I changed my mind";
  cout << "cow=" << cow.get() << endl;
  cout << "cow.owned=" << cow.owned() << endl;

  cout << "sizeof(A)=" << sizeof(A) << endl;
  char a[sizeof(A)];
  new(a) A();
  reinterpret_cast<A*>(a)->~A();
}

} // namespace

// `main` ---------------------------------------------------------------------------------------------------

void
myExit() {
  cout << "myExit" << endl;
  // throw InvalidState("Oopsers!");
}

void
myTerminate() {
  cout << "myTerminate" << endl;
  // throw 7;
}

extern const char* generated();

int
main(int argc, char **argv) {
  ROCKET_PROCESS_ERROR("Testing error before `process.init` ...");

  process.atExit(myExit);
  process.atExit(myTerminate, true);

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
    ROCKET_LOG_INFO("Hey {}", "there");
    auto& out = nio::stdout;
    out.println("This is {}", process.name());
    out.println("{}", generated());
    out.println("args: {}", args);
    toy();
  }

  process.exit(EXIT_SUCCESS);
}

// EOF
