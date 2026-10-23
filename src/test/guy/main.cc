/*
 * main.cc
 *
 * The `guy` test executable links to Rocket.Gui and is a playground for quick and dirty experiments.
 */

#include <rocket/Process.h>
#include <rocket/nio/nio.h>
#include <rocket-gui/rocket-gui.h>

using namespace rocket;
using namespace rocket::gui;

// `main` ---------------------------------------------------------------------------------------------------

i32
main(i32 argc, char **argv) {
  process.init(argc, argv, "guyy");
  nio::out.println("This is {}", process.name());
  howdy();
  process.exit(EXIT_SUCCESS);
}

// EOF
