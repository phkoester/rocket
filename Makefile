#
# Makefile
#

export INCLUDE_DIRS := main test
export SYSTEM_INCLUDE_DIRS := \
    $(GAIA_BOOST_DIR) \
    $(GAIA_CPP_UNICODELIB_DIR) \
    $(GAIA_GTEST_DIR)/googlemock/include \
    $(GAIA_GTEST_DIR)/googletest/include

# 'build' must be the first target and build everything, including tests
build: buildTest

include $(GAIA_DIR)/src/main/make/Makefile.mk

export ROCKET_VERSION := $(call version,.)

all: check doc test

buildMain:
	@$(call printInfo,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/main/Makefile build
	@echo Done.

buildTest: buildMain
	@$(call printInfo,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/test/Makefile build
	@echo Done.

doc: docMain docTest

docMain:
	@$(call printInfo,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/main/Makefile doc
	@echo Done.

docTest:
	@$(call printInfo,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/test/Makefile doc
	@echo Done.

test: buildMain
	@$(call printInfo,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/test/Makefile test
	@echo Done.

tests: buildMain
	@$(call printInfo,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/test/Makefile tests
	@echo Done.

# -----------------------------------------------------------------------------------------------------------

SHARED_LIB_DIRS := $(BUILD_DIR)

bare: buildMain
	@$(BUILD_DIR)/bare $(ARGS)

print-args: buildMain
	@$(SHARED_LIB_PATH_NAME)="$(SHARED_LIB_PATH):$(subst $(SPACE),:,$(SHARED_LIB_DIRS))" \
	 $(BUILD_DIR)/print-args $(ARGS)

print\ args: buildMain
	@$(SHARED_LIB_PATH_NAME)="$(SHARED_LIB_PATH):$(subst $(SPACE),:,$(SHARED_LIB_DIRS))" \
	 "$(BUILD_DIR)/print args" $(ARGS)

toy: buildMain
	@$(SHARED_LIB_PATH_NAME)="$(SHARED_LIB_PATH):$(subst $(SPACE),:,$(SHARED_LIB_DIRS))" \
	 $(BUILD_DIR)/toy $(ARGS)

.PHONY: crank

# EOF
