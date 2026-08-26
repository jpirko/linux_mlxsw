# SPDX-License-Identifier: GPL-2.0
CFLAGS += -Wall -O2 -Wno-unused-function
CFLAGS += $(KHDR_INCLUDES)
LDLIBS += -lpthread

TEST_GEN_PROGS := ctlv ctlv_bent

# Optional prerequisites are reported as skips rather than disappearing.
TEST_PROGS := compat32.sh ctlvctl.sh

# The same ioctls from a caller of the other ABI, where the toolchain can build
# one. That means -m32 on the architectures whose second ABI is a flag away;
# elsewhere, arm64 included, it is a different compiler rather than a flag, and
# this leg is a skip. The probe links as well as compiles, because a compiler
# that takes the flag without a 32-bit libc beside it would fail later and less
# clearly. Not in TEST_GEN_PROGS: compat32.sh is what runs it, so a machine
# that cannot build it says so instead of saying nothing.
CTLV_COMPAT_FLAG := $(shell echo 'int main(void){return 0;}' | \
	$(CC) -m32 -x c - -o /dev/null >/dev/null 2>&1 && echo -m32)
ifneq ($(CTLV_COMPAT_FLAG),)
TEST_GEN_PROGS_EXTENDED := ctlv_compat32
endif

# What ctlvctl.sh runs, and the names it reads, so that they are installed
# beside it.
TEST_GEN_PROGS_EXTENDED += ctlvctl
CTLV_MANIFESTS := $(patsubst %.yaml,%.json, \
	$(notdir $(wildcard ../../../../Documentation/ctlv/specs/*.yaml)))
TEST_GEN_FILES += $(CTLV_MANIFESTS)

LIBCTLV_DIR := ../../../ctlv/libctlv
CTLVCTL_DIR := ../../../ctlv/ctlvctl

EXTRA_CLEAN = $(OUTPUT)/libctlv $(OUTPUT)/ctlvctl.o $(OUTPUT)/spec.o

include ../lib.mk

LIBCTLV := $(OUTPUT)/libctlv/libctlv.a

$(OUTPUT)/ctlv $(OUTPUT)/ctlv_bent: ctlv_tlv.h ctlv_util.h

# Built for the other ABI, and against the same UAPI headers as everything
# here: what the layout assertions in ctlv_util.h are for is that this
# translation unit refuses to compile if the two ABIs disagree.
$(OUTPUT)/ctlv_compat32: ctlv_compat32.c ctlv_tlv.h ctlv_util.h
	$(CC) $(CTLV_COMPAT_FLAG) $(CFLAGS) $(CPPFLAGS) $< -o $@

$(LIBCTLV): FORCE
	$(Q)mkdir -p $(OUTPUT)/libctlv
	$(Q)$(MAKE) -C $(LIBCTLV_DIR) CC="$(CC)" OUTPUT=$(OUTPUT)/libctlv/ \
		KHDR_INCLUDES="$(KHDR_INCLUDES)"

$(OUTPUT)/ctlvctl: $(LIBCTLV) FORCE
	$(Q)$(MAKE) -C $(CTLVCTL_DIR) CC="$(CC)" OUTPUT=$(OUTPUT)/ \
		KHDR_INCLUDES="$(KHDR_INCLUDES)"

$(addprefix $(OUTPUT)/,$(CTLV_MANIFESTS)): $(OUTPUT)/ctlvctl ;

FORCE:

.PHONY: FORCE
