#
# Makefile
#
# Parameters:
#
# - GAIA_DIR
#
# Targets:
#
# - test-terminal
#

DEFAULT_RUN_TARGET := toy

ifndef GAIA_DIR
  $(error `GAIA_DIR` not set)
endif
include $(GAIA_DIR)/src/main/make/Makefile-C.mk

# Targets ---------------------------------------------------------------------------------------------------

.PHONY: test-terminal
test-terminal: build
	@ROCKET_TEST_TERMINAL=1 $(TEST_DIR)/test-rocket-system-terminal
	@ROCKET_TEST_TERMINAL=1 $(TEST_DIR)/test-rocket-unicode-Character

# EOF
