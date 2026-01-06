#
# Makefile
#

export INCLUDE_DIRS := main test
export SYSTEM_INCLUDE_DIRS := \
    $(GAIA_BOOST_DIR) \
    $(GAIA_CPP_UNICODELIB_DIR) \
    $(GAIA_FMT_DIR)/include \
    $(GAIA_GOOGLETEST_DIR)/googlemock/include \
    $(GAIA_GOOGLETEST_DIR)/googletest/include \
    $(GAIA_SCNLIB_DIR)/include

# `build` must be the first target and build everything, including benches and tests
build: buildBench buildTest

include $(GAIA_DIR)/src/main/make/Makefile.mk

export ROCKET_VERSION := $(call print-version,.)
ifeq ($(ROCKET_VERSION),)
  $(error Cannot set `ROCKET_VERSION`)
endif

all: check doc test bench

bench: buildMain
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/bench/Makefile bench

benches: buildMain
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/bench/Makefile benches

buildBench: buildMain
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/bench/Makefile build

buildMain:
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/main/Makefile build

buildTest: buildMain
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/test/Makefile build

doc: docMain docTest

docMain:
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/main/Makefile doc

docTest:
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/test/Makefile doc

test: buildMain
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/test/Makefile test

tests: buildMain
	@$(call print-info,$@)
	@+$(MAKE) $(MAKE_FLAGS) -f src/test/Makefile tests

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
