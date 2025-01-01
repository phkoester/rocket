#
# Makefile
#

COMPILE_DEPS := src/main/rocket/version.h

include $(GAIA_DIR)/src/main/make/Makefile.mk

# 'bare' ----------------------------------------------------------------------------------------------------

NAME     := bare
CC_FILES := $(foreach it,$(wildcard src/main/bare/*.cc),$(it:src/%=%))
include $(GAIA_DIR)/src/main/make/executable.mk

# 'print args' ----------------------------------------------------------------------------------------------

$(BUILD_DIR)/print\ args$(EXECUTABLE_SUFFIX): $(BUILD_DIR)/print-args$(EXECUTABLE_SUFFIX)
	@echo ">" $@
	@cp $< "$@"

printArgsWithSpace: $(BUILD_DIR)/print\ args$(EXECUTABLE_SUFFIX)

.PHONY: printArgsWithSpace

# 'print-args' ----------------------------------------------------------------------------------------------

NAME        := print-args
CC_FILES    := $(foreach it,$(wildcard src/main/print-args/*.cc),$(it:src/%=%))
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/executable.mk

# 'rocket' --------------------------------------------------------------------------------------------------

$(BUILD_DIR)/main/rocket/unicode-EastAsianWidth.cc: src/main/rocket/EastAsianWidth.txt
	@echo ">" $@
	@python3 -c 'from gaia.unicode import *; parseEastAsianWidth("$<", "$@")'

$(BUILD_DIR)/main/rocket/unicode-Emoji.cc: src/main/rocket/emoji-data.txt
	@echo ">" $@
	@python3 -c 'from gaia.unicode import *; parseEmoji("$<", "$@")'

NAME     := rocket
VERSION  := $(ROCKET_VERSION)
# Move 'log.cc' to front so Rocket can use its own logging API
# Move 'Process.cc' to front so the global 'process' instance is available early
CC_FILES := $(call moveFront, \
    main/rocket/log.cc main/rocket/Process.cc, \
    $(foreach it,$(wildcard src/main/rocket/*.cc),$(it:src/%=%))) \
    main/rocket/unicode-EastAsianWidth.cc \
    main/rocket/unicode-Emoji.cc
include $(GAIA_DIR)/src/main/make/shared-lib.mk

# 'rocket-gtest' --------------------------------------------------------------------------------------------

NAME     := rocket-gtest
CC_FILES := $(foreach it,$(wildcard src/main/rocket-gtest/*.cc),$(it:src/%=%))
include $(GAIA_DIR)/src/main/make/lib.mk

# 'toy' -----------------------------------------------------------------------------------------------------

NAME        := toy
CC_FILES    := $(foreach it,$(wildcard src/main/toy/*.cc),$(it:src/%=%))
SHARED_LIBS := $(call sharedLibName,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/executable.mk

# 'version.h' -----------------------------------------------------------------------------------------------

src/main/rocket/version.h: ROCKET_HEX_VERSION := $(call hexVersion,.)
src/main/rocket/version.h: ROCKET_VERSION_CODE := $(call versionCode,.)
src/main/rocket/version.h: project.json $(MAKEFILE_DEPS)
	@echo ">" $@
	@sed -Ei \
	    -e 's|(.*)_VERSION .* // .*|\1_VERSION $(ROCKET_HEX_VERSION) // $(ROCKET_VERSION)|' \
	    -e 's|(.*)_VERSION_CODE .*|\1_VERSION_CODE $(ROCKET_VERSION_CODE)|' \
            $@

# All -------------------------------------------------------------------------------------------------------

build: $(BUILD_DEPS) printArgsWithSpace

doc:
	@mkdir -p $(BUILD_DIR)/main/doc
	@$(DOXYGEN) $(DOXYGEN_FLAGS) src/main/Doxyfile

# EOF
