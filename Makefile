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
#     `all` , `bench`, `test` (default), or PATTERN
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
#
# - Targets for test executables:
#
# - bare
# - logger
# - print-args
# - toy
#

# The default target
.PHONY: build
build: cmake-build

COMPILE_DEPS := $(wildcard $(GAIA_DIR)/bin/*) $(wildcard $(GAIA_DIR)/src/main/cmake/*)

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

# Test executables ------------------------------------------------------------------------------------------

.PHONY: bare
bare: TARGET := bare
bare: build
	@$(BUILD_DIR)/src/test/bare $(ARGS)

.PHONY: logger
logger: TARGET := logger
logger: build
	@$(BUILD_DIR)/src/test/logger $(ARGS)

.PHONY: print-args
print-args: TARGET := print-args
print-args: build
	@$(BUILD_DIR)/src/test/print-args $(ARGS)

.PHONY: toy
toy: TARGET := toy
toy: build
	@$(BUILD_DIR)/src/test/toy $(ARGS)

# EOF
