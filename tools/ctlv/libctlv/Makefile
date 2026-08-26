# SPDX-License-Identifier: GPL-2.0

KDIR ?= ../../..

CFLAGS ?= -O2 -g

include ../../scripts/Makefile.include

# What make headers_install leaves behind, which is where <ctlv/ctlv.h> and
# every generated family header come from.
KHDR_INCLUDES ?= -isystem $(or $(ABSOLUTE_O),$(KDIR))/usr/include

CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -D_GNU_SOURCE -I. $(KHDR_INCLUDES)

LIB = $(OUTPUT)libctlv.a

# ctlv_user_gen.o is the root family's specification, generated from the same
# YAML the core is generated from: the ops this library implements directly are
# described by the tool that describes every other family's.
OBJS = $(addprefix $(OUTPUT),	\
	ctlv_user_gen.o			\
	device.o			\
	introspection.o			\
	message.o			\
	schema.o			\
	spec.o				\
	transport.o)

all: $(LIB)

$(LIB): $(OBJS)
	$(QUIET_AR)$(AR) rcs $@ $^

$(OUTPUT)%.o: %.c ctlv.h internal.h
	$(QUIET_CC)$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
