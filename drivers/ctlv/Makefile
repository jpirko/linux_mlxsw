# SPDX-License-Identifier: GPL-2.0
# The test device and the generated kernel interface of its family.
obj-$(CONFIG_CTLV_TEST) += ctlv-test.o

ctlv-test-y += tests/ctlv_test.o \
	       tests/ctlv_test_gen.o

# The same for the family inheriting it, a module of its own so that the chain
# is shown crossing one.
obj-$(CONFIG_CTLV_TEST_CHILD) += ctlv-test-child.o

ctlv-test-child-y += tests/ctlv_test_child.o \
		     tests/ctlv_test_child_gen.o

# The test family again, on a file whose subsystem is not CTLV.
obj-$(CONFIG_CTLV_TEST_MISC) += ctlv-test-misc.o

ctlv-test-misc-y += tests/ctlv_test_misc.o
