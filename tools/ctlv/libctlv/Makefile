# SPDX-License-Identifier: GPL-2.0

KDIR ?= ../../..

# What make headers_install leaves behind, which is where <linux/ctlv.h> and
# every generated family header come from.
KHDR_INCLUDES ?= -isystem $(KDIR)/usr/include

CFLAGS ?= -O2 -g
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -D_GNU_SOURCE -I. $(KHDR_INCLUDES)

LIB = libctlv.a

# ctlv_user_gen.o is the root family's specification, generated from the same
# YAML the core is generated from: the ops this library implements by hand are
# described by the tool that describes every other family's.
OBJS =					\
	ctlv_user_gen.o			\
	device.o			\
	event.o				\
	introspection.o			\
	message.o			\
	schema.o			\
	spec.o				\
	transport.o

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(OBJS): ctlv.h internal.h

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
