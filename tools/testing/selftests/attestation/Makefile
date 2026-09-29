# SPDX-License-Identifier: GPL-2.0
CFLAGS += -Wall -O2 $(KHDR_INCLUDES)
CFLAGS += -I../../../ctlv/libctlv
LDLIBS += -lpthread

TEST_GEN_PROGS := attestation_test

LIBCTLV := ../../../ctlv/libctlv/libctlv.a
BINDINGS := ctlv_attestation_user_gen.c ctlv_attestation_sim_user_gen.c

include ../lib.mk

$(OUTPUT)/attestation_test: attestation_test.c $(BINDINGS) $(LIBCTLV)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ $(LDLIBS) -o $@

$(LIBCTLV):
	$(MAKE) -C ../../../ctlv/libctlv
