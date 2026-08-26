# SPDX-License-Identifier: GPL-2.0

KDIR ?= ../../..
LIBDIR = ../libctlv

CFLAGS ?= -O2 -g

include ../../scripts/Makefile.include

KHDR_INCLUDES ?= -isystem $(or $(ABSOLUTE_O),$(KDIR))/usr/include

CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -D_GNU_SOURCE -I$(LIBDIR) $(KHDR_INCLUDES)

# The one that knows the target's json-c, when cross-compiling.
PKG_CONFIG ?= $(CROSS_COMPILE)pkg-config

JSON_C_CFLAGS := $(shell $(PKG_CONFIG) --atleast-version=0.14 json-c \
			  2>/dev/null && $(PKG_CONFIG) --cflags json-c)
JSON_C_LIBS := $(shell $(PKG_CONFIG) --atleast-version=0.14 json-c \
			2>/dev/null && $(PKG_CONFIG) --libs json-c)
ifneq ($(JSON_C_LIBS),)
CFLAGS += -DHAVE_JSON_C $(JSON_C_CFLAGS)
LDLIBS += $(JSON_C_LIBS)
endif

# With a build directory, libctlv is built for ctlvctl inside it.
LIBCTLV_OUTPUT ?= $(if $(OUTPUT),$(OUTPUT)libctlv/)
LIB = $(or $(LIBCTLV_OUTPUT),$(LIBDIR)/)libctlv.a
OBJS = $(OUTPUT)ctlvctl.o $(OUTPUT)spec.o

# What --spec-dir reads where there is neither the source tree nor Python:
# the names of each of the tree's families and of the families it inherits.
DOCDIR = $(KDIR)/Documentation/ctlv
SPECDIR = $(DOCDIR)/specs
SPECS = $(wildcard $(SPECDIR)/*.yaml)
CTLVGEN = ../ctlvgen/cli.py
CTLVGEN_INPUTS = $(SPECS) $(wildcard $(DOCDIR)/*.yaml) \
		 $(wildcard ../ctlvgen/*.py ../ctlvgen/lib/*.py)
MANIFESTS = $(patsubst $(SPECDIR)/%.yaml,$(OUTPUT)%.json,$(SPECS))

all: $(OUTPUT)ctlvctl $(MANIFESTS)

$(OUTPUT)ctlvctl: $(OBJS) $(LIB)
	$(QUIET_LINK)$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIB) $(LDLIBS)

$(OUTPUT)%.o: %.c spec.h
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<

$(OUTPUT)%.json: $(SPECDIR)/%.yaml $(CTLVGEN_INPUTS)
	$(QUIET_GEN)$(CTLVGEN) --spec $< --runtime-json > $@

$(LIB): FORCE
	$(if $(LIBCTLV_OUTPUT),$(Q)mkdir -p $(LIBCTLV_OUTPUT))
	$(Q)$(MAKE) -C $(LIBDIR) OUTPUT=$(LIBCTLV_OUTPUT)

clean:
	rm -f $(OUTPUT)ctlvctl $(OBJS) $(MANIFESTS)
	$(if $(LIBCTLV_OUTPUT),rm -rf $(LIBCTLV_OUTPUT))

FORCE:

.PHONY: all clean FORCE
.DELETE_ON_ERROR:
