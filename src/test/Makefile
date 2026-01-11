#
# Makefile
#

include $(GAIA_DIR)/src/main/make/Makefile.mk

# `rocket` --------------------------------------------------------------------------------------------------

NAME        := test-rocket-Exception
CC_FILES    := test/rocket/test-Exception.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-Guard
CC_FILES    := test/rocket/test-Guard.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-Process
CC_FILES    := test/rocket/test-Process.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-Type
CC_FILES    := test/rocket/test-Type.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-UnorderedBimap
CC_FILES    := test/rocket/test-UnorderedBimap.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-assert
CC_FILES    := test/rocket/test-assert.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-enum
CC_FILES    := test/rocket/test-enum.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-hash
CC_FILES    := test/rocket/test-hash.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-macro
CC_FILES    := test/rocket/test-macro.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-numeric
CC_FILES    := test/rocket/test-numeric.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-rocket
CC_FILES    := test/rocket/test-rocket.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/3rdparty` .........................................................................................

NAME        := test-rocket-3rdparty-C++
CC_FILES    := test/rocket/3rdparty/test-C++.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-3rdparty-boost-spirit-x3
CC_FILES    := test/rocket/3rdparty/test-boost-spirit-x3.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-3rdparty-fmt
CC_FILES    := test/rocket/3rdparty/test-fmt.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-3rdparty-icu
CC_FILES    := test/rocket/3rdparty/test-icu.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB) libicuuc.so.$(ICU_VERSION)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-3rdparty-scnlib
CC_FILES    := test/rocket/3rdparty/test-scnlib.cc
LIB_FILES   := $(INSTALL_LIB_DIR)/libscn$(G).a
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-3rdparty-std
CC_FILES    := test/rocket/3rdparty/test-std.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/cl` ...............................................................................................

NAME        := test-rocket-cl
CC_FILES    := test/rocket/cl/test-cl.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/format` ...........................................................................................

NAME        := test-rocket-format
CC_FILES    := test/rocket/format/test-format.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-format-std
CC_FILES    := test/rocket/format/test-std.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/log` ..............................................................................................

NAME        := test-rocket-log
CC_FILES    := test/rocket/log/test-log.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/math` .............................................................................................

NAME        := test-rocket-math
CC_FILES    := test/rocket/math/test-math.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-math-Interval
CC_FILES    := test/rocket/math/test-Interval.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/nio` ..............................................................................................

NAME        := test-rocket-nio
CC_FILES    := test/rocket/nio/test-nio.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/reflect` ..........................................................................................

NAME        := test-rocket-reflect
CC_FILES    := test/rocket/reflect/test-reflect.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/str` ..............................................................................................

NAME        := test-rocket-str
CC_FILES    := test/rocket/str/test-str.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-str-StringConvert
CC_FILES    := test/rocket/str/test-StringConvert.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-str-escape
CC_FILES    := test/rocket/str/escape/test-escape.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-str-location
CC_FILES    := test/rocket/str/location/test-location.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/system` ...........................................................................................

NAME        := test-rocket-system
CC_FILES    := test/rocket/system/test-system.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-system-terminal
CC_FILES    := test/rocket/system/terminal/test-terminal.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket/unicode` ..........................................................................................

NAME        := test-rocket-unicode
CC_FILES    := test/rocket/unicode/test-unicode.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-unicode-iterator
CC_FILES    := test/rocket/unicode/test-iterator.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# `rocket-gtest` --------------------------------------------------------------------------------------------

NAME        := test-rocket-gtest-matcher
CC_FILES    := test/rocket-gtest/matcher/test-matcher.cc
SHARED_LIBS := $(ROCKET_SHARED_LIB)
include $(GAIA_DIR)/src/main/make/test-executable.mk

# All -------------------------------------------------------------------------------------------------------

build: $(BUILD_DEPS)

doc:
	@mkdir -p $(BUILD_DIR)/test/doc
	@$(DOXYGEN) $(DOXYGEN_FLAGS) src/test/Doxyfile

test: $(TEST_DEPS)

TESTS_DEPS := $(addprefix test-,$(TESTS))

tests: $(TESTS_DEPS)

# EOF
