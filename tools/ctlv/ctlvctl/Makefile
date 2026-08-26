# SPDX-License-Identifier: GPL-2.0

KDIR ?= ../../..
LIBDIR = ../libctlv

KHDR_INCLUDES ?= -isystem $(KDIR)/usr/include

CFLAGS ?= -O2 -g
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -D_GNU_SOURCE -I$(LIBDIR) $(KHDR_INCLUDES)

LIB = $(LIBDIR)/libctlv.a

all: ctlvctl

ctlvctl: ctlvctl.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< $(LIB)

$(LIB):
	$(MAKE) -C $(LIBDIR)

clean:
	rm -f ctlvctl

.PHONY: all clean $(LIB)
