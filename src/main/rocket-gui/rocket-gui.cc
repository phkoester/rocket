/*
 * rocket-gui.cc
 */

#include "rocket-gui.h"

#include <rocket/nio/nio.h>

namespace rocket::gui {

void howdy() {
  nio::out.print("Howdy!");
}

} // namespace rocket::gui

// EOF
