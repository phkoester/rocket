#
# Makefile
#
# Parameters:
#
# - GAIA_DIR
#

DEFAULT_RUN_TARGET := toy

ifndef GAIA_DIR
  $(error `GAIA_DIR` not set)
endif
include $(GAIA_DIR)/src/main/make/Makefile-C.mk

# EOF
