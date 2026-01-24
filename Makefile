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
#     Number of jobs for gmake
# - TEST
#     Run tests matching the pattern
# - VERBOSE
#     Produce verbose output
#
# This Makefile will only work in Linux, with Gaia installed and properly configured. It's for active
# development purposes only.
#
# To build the project using CMake, see `README.md`.
#

export BUILD_DIR := build/$(GAIA_BUILD_TYPE)
PRESET := linux-$(GAIA_BUILD_TYPE)

ifeq ($(GAIA_CXX_TOOLCHAIN),llvm)
  export CC := clang
  export CXX := clang++
endif

ifneq ($(VERBOSE),)
  CMAKE_TRAILING_FLAGS += -v
endif

GMAKE_FLAGS := --no-print-directory
ifneq ($(JOBS),)
  ifneq ($(JOBS),0)
    GMAKE_FLAGS += -j$(JOBS)
  endif
else
  GMAKE_FLAGS += -j$(GAIA_NPROC_2_3) -l$(GAIA_NPROC)
endif

.PHONY: build
build: $(BUILD_DIR)/Makefile
	cmake $(CMAKE_FLAGS) --build --preset $(PRESET) $(CMAKE_TRAILING_FLAGS) -- $(GMAKE_FLAGS)

$(BUILD_DIR)/Makefile: $(shell find -name CMakeLists.txt) $(shell find cmake -name "*.cmake")
	cmake $(CMAKE_FLAGS) --preset $(PRESET)

$(BUILD_DIR)/compile_commands.json: build

compile_commands.json: $(BUILD_DIR)/compile_commands.json
	@echo ">" $@
	@cp $< $@

.PHONY: clean
clean:
	rm -rf build install

.PHONY: bare
bare: build
	LD_LIBRARY_PATH=$(BUILD_DIR)/src/main:$(LD_LIBRARY_PATH) \
	$(BUILD_DIR)/src/main/bare $(ARGS)

.PHONY: print-args
print-args: build
	LD_LIBRARY_PATH=$(BUILD_DIR)/src/main:$(LD_LIBRARY_PATH) \
	$(BUILD_DIR)/src/main/print-args $(ARGS)

.PHONY: print-args-with-space
print-args-with-space: build
	LD_LIBRARY_PATH=$(BUILD_DIR)/src/main:$(LD_LIBRARY_PATH) \
	"$(BUILD_DIR)/src/main/print args" $(ARGS)

.PHONY: toy
toy: build
	LD_LIBRARY_PATH=$(BUILD_DIR)/src/main:$(LD_LIBRARY_PATH) \
	$(BUILD_DIR)/src/main/toy $(ARGS)

CTEST_FLAGS := --output-on-failure
ifneq ($(TEST),)
  CTEST_FLAGS += -R $(TEST)
endif
ifneq ($(VERBOSE),)
  CTEST_FLAGS += -V
endif

.PHONY: test
test: build
	ctest $(CTEST_FLAGS) --preset $(PRESET)

# Manual install to `/usr/local`:
#
#   sudo cmake --install build/$GAIA_BUILD_TYPE
.PHONY: install
install: build
	cmake --install $(BUILD_DIR) --prefix install

# EOF
