# SPDX-License-Identifier: GPL-2.0

KDIR ?= ../../..
LIBDIR = ../libctlv

KHDR_INCLUDES ?= -isystem $(KDIR)/usr/include

CFLAGS ?= -O2 -g
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -D_GNU_SOURCE -I$(LIBDIR) $(KHDR_INCLUDES)

JSON_C_CFLAGS := $(shell pkg-config --atleast-version=0.14 json-c 2>/dev/null \
			  && pkg-config --cflags json-c)
JSON_C_LIBS := $(shell pkg-config --atleast-version=0.14 json-c 2>/dev/null \
			&& pkg-config --libs json-c)
ifneq ($(JSON_C_LIBS),)
CFLAGS += -DHAVE_JSON_C $(JSON_C_CFLAGS)
LDLIBS += $(JSON_C_LIBS)
endif

LIB = $(LIBDIR)/libctlv.a
OBJS = ctlvctl.o spec.o

all: ctlvctl

ctlvctl: $(OBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIB) $(LDLIBS)

$(OBJS): spec.h

$(LIB):
	$(MAKE) -C $(LIBDIR)

clean:
	rm -f ctlvctl $(OBJS)

.PHONY: all clean $(LIB)
