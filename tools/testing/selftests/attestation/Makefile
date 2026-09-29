# SPDX-License-Identifier: GPL-2.0
CFLAGS += -Wall -O2 $(KHDR_INCLUDES)
CFLAGS += -I../../../ctlv/libctlv
LDLIBS += -lpthread

TEST_GEN_PROGS := attestation_test

LIBCTLV_DIR := ../../../ctlv/libctlv
BINDINGS := ctlv_attestation_user_gen.c ctlv_attestation_sim_user_gen.c

EXTRA_CLEAN = $(OUTPUT)/libctlv

include ../lib.mk

LIBCTLV := $(OUTPUT)/libctlv/libctlv.a

$(OUTPUT)/attestation_test: attestation_test.c $(BINDINGS) $(LIBCTLV)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ $(LDLIBS) -o $@

$(LIBCTLV): FORCE
	$(Q)mkdir -p $(OUTPUT)/libctlv
	$(Q)$(MAKE) -C $(LIBCTLV_DIR) CC="$(CC)" OUTPUT=$(OUTPUT)/libctlv/ \
		KHDR_INCLUDES="$(KHDR_INCLUDES)"

FORCE:

.PHONY: FORCE
