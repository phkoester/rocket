#
# Makefile
#

include $(GAIA_DIR)/src/main/make/Makefile.mk

# `rocket` --------------------------------------------------------------------------------------------------

NAME        := bench-rocket-codec-std
CC_FILES    := bench/rocket/bench-codec-std.cc
SHARED_LIBS := $(call shared-lib-name,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/bench-executable.mk

NAME        := bench-rocket-io
CC_FILES    := bench/rocket/bench-io.cc
SHARED_LIBS := $(call shared-lib-name,rocket,$(ROCKET_VERSION))
include $(GAIA_DIR)/src/main/make/bench-executable.mk

# All -------------------------------------------------------------------------------------------------------

build: $(BUILD_DEPS)

bench: $(BENCH_DEPS)

BENCHES_DEPS := $(addprefix bench-,$(BENCHES))

benches: $(BENCHES_DEPS)

# EOF
