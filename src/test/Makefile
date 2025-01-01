#
# Makefile
#

include $(GAIA_DIR)/src/main/make/Makefile.mk

# 'rocket' --------------------------------------------------------------------------------------------------

NAME        := test-rocket-C++
CC_FILES    := test/rocket/test-C++.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-S
CC_FILES    := test/rocket/test-S.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-StringConvert
CC_FILES    := test/rocket/test-StringConvert.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-Type
CC_FILES    := test/rocket/test-Type.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-assert
CC_FILES    := test/rocket/test-assert.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-basic
CC_FILES    := test/rocket/test-basic.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-boost-spirit-x3
CC_FILES    := test/rocket/test-boost-spirit-x3.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-cl
CC_FILES    := test/rocket/test-cl.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-codec
CC_FILES    := test/rocket/test-codec.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-codec-boost
CC_FILES    := test/rocket/test-codec-boost.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-codec-global
CC_FILES    := test/rocket/test-codec-global.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-codec-rocket
CC_FILES    := test/rocket/test-codec-rocket.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-codec-std
CC_FILES    := test/rocket/test-codec-std.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-enum
CC_FILES    := test/rocket/test-enum.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-escape
CC_FILES    := test/rocket/test-escape.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-except
CC_FILES    := test/rocket/test-except.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-io
CC_FILES    := test/rocket/test-io.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-log
CC_FILES    := test/rocket/test-log.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-macro
CC_FILES    := test/rocket/test-macro.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-math
CC_FILES    := test/rocket/test-math.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-reflect
CC_FILES    := test/rocket/test-reflect.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-scoped
CC_FILES    := test/rocket/test-scoped.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-std
CC_FILES    := test/rocket/test-std.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-strings
CC_FILES    := test/rocket/test-strings.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-system
CC_FILES    := test/rocket/test-system.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-text
CC_FILES    := test/rocket/test-text.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

NAME        := test-rocket-unicode
CC_FILES    := test/rocket/test-unicode.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/test-executable.mk

# 'rocket-gtest' --------------------------------------------------------------------------------------------

NAME        := test-rocket-gtest-match
CC_FILES    := test/rocket-gtest/test-match.cc
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
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
