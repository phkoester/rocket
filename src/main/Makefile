#
# Makefile
#

COMPILE_DEPS := src/main/rocket/version.h

include $(GAIA_DIR)/src/main/make/Makefile.mk

# `version.h` -----------------------------------------------------------------------------------------------

src/main/rocket/version.h: gaia-project.json
	@echo ">" $@
	@$(call write-version-header,$@,ROCKET_VERSION,$(ROCKET_VERSION))

# `bare` ----------------------------------------------------------------------------------------------------

NAME    := bare
SRC_DIR := main/bare
include $(GAIA_DIR)/src/main/make/executable.mk

# `print args` ----------------------------------------------------------------------------------------------

$(BUILD_DIR)/print\ args$(EXECUTABLE_SUFFIX): $(BUILD_DIR)/print-args$(EXECUTABLE_SUFFIX)
	@echo ">" $@
	@cp $< "$@"

printArgsWithSpace: $(BUILD_DIR)/print\ args$(EXECUTABLE_SUFFIX)

.PHONY: printArgsWithSpace

# `print-args` ----------------------------------------------------------------------------------------------

NAME        := print-args
SRC_DIR     := main/print-args
SHARED_LIBS := $(call shared-lib-name,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/executable.mk

# `rocket` --------------------------------------------------------------------------------------------------

$(BUILD_DIR)/main/rocket/unicode/EastAsianWidth.cc: src/main/rocket/unicode/EastAsianWidth.txt
	@echo ">" $@
	@mkdir -p $(dir $@)
	@python3 -c 'from gaia.unicode import *; parse_east_asian_width("$<", "$@")'

$(BUILD_DIR)/main/rocket/unicode/emoji-data.cc: src/main/rocket/unicode/emoji-data.txt
	@echo ">" $@
	@mkdir -p $(dir $@)
	@python3 -c 'from gaia.unicode import *; parse_emoji("$<", "$@")'

NAME           := rocket
VERSION        := $(ROCKET_VERSION)
SRC_DIR        := main/rocket
SRC_ADD        := unicode/EastAsianWidth.cc unicode/emoji-data.cc
# Move `log.cc` to front so Rocket can use its own logging API
# Move `Process.cc` to front so the global `process` instance is available early
SRC_MOVE_FRONT := log/log.cc Process.cc
include $(GAIA_DIR)/src/main/make/shared-lib.mk

# `rocket-gtest` --------------------------------------------------------------------------------------------

NAME     := rocket-gtest
SRC_DIR  := main/rocket-gtest
include $(GAIA_DIR)/src/main/make/lib.mk

# `toy` -----------------------------------------------------------------------------------------------------

NAME        := toy
SRC_DIR     := main/toy
SHARED_LIBS := $(call shared-lib-name,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/executable.mk

# All -------------------------------------------------------------------------------------------------------

build: $(BUILD_DEPS) printArgsWithSpace

doc:
	@mkdir -p $(BUILD_DIR)/main/doc
	@$(DOXYGEN) $(DOXYGEN_FLAGS) src/main/Doxyfile

# EOF
