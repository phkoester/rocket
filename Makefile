#
# Makefile
#
# This Makefile will only work in Linux, with Gaia installed and properly configured. It's for active
# development purposes only.
#
# To build the project using CMake, see `README.md`.
#
# Parameters:
#
# - ARGS
#     Arguments to pass to executables
# - GAIA_BUILD_TYPE
#     The build type: `debug` or `release`
# - GAIA_CXX_TOOLCHAIN
#     The C++ toolchain: `gnu` or `llvm`
# - JOBS
#     Number of jobs for GNU Make (0: none, N: N jobs, default: 2/3 nproc)
# - TEST
#     `all` , `test` (default), `bench`, or PATTERN
# - VERBOSE
#     Produce verbose output
#
# Some inherited targets:
#
# - check
# - doc
# - info
#
# Build-related targets:
#
# - build (default)
# - configure
# - test
# - test-terminal
# - clean
# - patch
#
# Targets for executables:
#
# - bare
# - print-args
# - toy
#

# The default target
.PHONY: build
build: cmake-build

COMPILE_DEPS := make.cmd cmake/base.cmake cmake/generate-version.py

include $(GAIA_DIR)/src/main/make/Makefile.mk

# Build-related targets -------------------------------------------------------------------------------------

.PHONY: configure
configure: cmake-configure

.PHONY: test
test: cmake-test

.PHONY: test-terminal
test-terminal: build
	@ROCKET_TEST_TERMINAL=1 $(BUILD_DIR)/src/test/test-rocket-system-terminal
	@ROCKET_TEST_TERMINAL=1 $(BUILD_DIR)/src/test/test-rocket-unicode-Character

.PHONY: clean
clean:
	@rm -rfv build install

# `patch` ...................................................................................................

.PHONY: patch
patch: build src/main/rocket/3rdparty/fmt/std.h src/main/rocket/3rdparty/scnlib/impl.h

FMT_VERSION_EXPECTED := 12.1.0
ifneq ($(wildcard $(BUILD_DIR)/_deps/fmt-src),)
  FMT_VERSION = $(shell git -C $(BUILD_DIR)/_deps/fmt-src describe --tags --abbrev=0)
endif

src/main/rocket/3rdparty/fmt/std.h: $(BUILD_DIR)/_deps/fmt-src/include/fmt/std.h
ifneq ($(FMT_VERSION), $(FMT_VERSION_EXPECTED))
	@echo fmt version has changed from $(FMT_VERSION_EXPECTED) to $(FMT_VERSION)!
	@echo You have to copy and patch std.h manually!
	@echo
	@echo "  cp $< $@"
endif

.PHONY: src/main/rocket/3rdparty/scnlib/impl.h
src/main/rocket/3rdparty/scnlib/impl.h: $(BUILD_DIR)/_deps/scnlib-src/src/scn/impl.h
	@diff $< $@ >/dev/null || ( \
          echo The file impl.h in scnlib has changed!; \
	  echo You have to copy impl.h manually!; \
	  echo; \
	  echo "  cp $< $@"; \
	  echo; \
	  echo You have to patch src/main/rocket/3rdparty/scnlib/impl.cc manually! \
	)


# Executables -----------------------------------------------------------------------------------------------

.PHONY: bare
bare: TARGET := bare
bare: build
	@$(BUILD_DIR)/src/main/bare $(ARGS)

.PHONY: print-args
print-args: TARGET := print-args
print-args: build
	@$(BUILD_DIR)/src/main/print-args $(ARGS)

.PHONY: toy
toy: TARGET := toy
toy: build
	@$(BUILD_DIR)/src/main/toy $(ARGS)

# EOF
