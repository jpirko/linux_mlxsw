# SPDX-License-Identifier: GPL-2.0
# The test device and the generated kernel interface of its family.
obj-$(CONFIG_CTLV_TEST) += ctlv-test.o

ctlv-test-y += tests/ctlv_test.o \
	       tests/ctlv_test_gen.o

# The test family again, on a file whose subsystem is not CTLV.
obj-$(CONFIG_CTLV_TEST_MISC) += ctlv-test-misc.o

ctlv-test-misc-y += tests/ctlv_test_misc.o
