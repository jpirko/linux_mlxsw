# SPDX-License-Identifier: GPL-2.0

CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -I../../../usr/include

all: ctlvbench

ctlvbench: ctlvbench.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f ctlvbench

.PHONY: all clean
