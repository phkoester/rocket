#
# Makefile
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
#     Run tests matching the pattern
# - VERBOSE
#     Produce verbose output
#
# Targets:
#
# - build (default)
# - check (inherited)
# - clean
# - doc
# - patch
# - test
# - test-terminal
# - install
#
# Targets for executables:
#
# - bare
# - print-args
# - toy
#
# This Makefile will only work in Linux, with Gaia installed and properly configured. It's for active
# development purposes only.
#
# To build the project using CMake, see `README.md`.
#

# The default target
.PHONY: build
build: compile_commands.json

COMPILE_DEPS := build.cmd cmake/base.cmake

include $(GAIA_DIR)/src/main/make/Makefile.mk

.PHONY: clean
clean:
	@rm -rfv build install

.PHONY: doc
doc:
	@mkdir -p $(BUILD_DIR)/src/main/doc
	@doxygen $(DOXYGEN_FLAGS) src/main/Doxyfile
	@mkdir -p $(BUILD_DIR)/src/test/doc
	@doxygen $(DOXYGEN_FLAGS) src/test/Doxyfile

.PHONY: test
test: cmake-test

.PHONY: test-terminal
test-terminal: build
	@ROCKET_TEST_TERMINAL=1 $(BUILD_DIR)/src/test/test-rocket-system-terminal
	@ROCKET_TEST_TERMINAL=1 $(BUILD_DIR)/src/test/test-rocket-unicode-Character

# Manual install to `/usr/local`:
#
#   sudo cmake --install build/$GAIA_BUILD_TYPE
.PHONY: install
install: build
	cmake --install $(BUILD_DIR) --prefix install

.PHONY: bare
bare: build
	@LD_LIBRARY_PATH=$(BUILD_DIR)/src/main:$(LD_LIBRARY_PATH) \
	  $(BUILD_DIR)/src/main/bare $(ARGS)

.PHONY: print-args
print-args: build
	@LD_LIBRARY_PATH=$(BUILD_DIR)/src/main:$(LD_LIBRARY_PATH) \
	  $(BUILD_DIR)/src/main/print-args $(ARGS)

.PHONY: toy
toy: build
	@LD_LIBRARY_PATH=$(BUILD_DIR)/src/main:$(LD_LIBRARY_PATH) \
	  $(BUILD_DIR)/src/main/toy $(ARGS)

# Patch -----------------------------------------------------------------------------------------------------

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
src/main/rocket/3rdparty/scnlib/impl.h: $(BUILD_DIR)/_deps/scn-src/src/scn/impl.h
	@diff $< $@ >/dev/null || ( \
          echo The file impl.h in scnlib has changed!; \
	  echo You have to copy impl.h manually!; \
	  echo; \
	  echo "  cp $< $@"; \
	  echo; \
	  echo You have to patch src/main/rocket/3rdparty/scnlib/impl.cc manually! \
	)

# EOF
