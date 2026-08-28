# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_CTLV) += ctlv.o

ctlv-y += blob.o core.o event.o introspection.o message.o schema.o \
	  ctlv_gen.o

# The suites are one module of their own, so that testing the framework does
# not change the framework that ships. The fixture families exist to exercise
# the schema tool, and their generated metadata is what the suites walk beside
# the core family's.
obj-$(CONFIG_CTLV_KUNIT_TEST) += ctlv-kunit.o

ctlv-kunit-y += tests/ctlv_kunit_mod.o \
		tests/blob_kunit.o \
		tests/event_kunit.o \
		tests/message_kunit.o \
		tests/metadata_kunit.o \
		tests/registration_kunit.o \
		tests/ctlv_fixture_gen.o \
		tests/ctlv_fixture_child_gen.o

# A fixture has the generated UAPI header of a family that is not installed, so
# the tests that read one say where a fixture keeps its linux/ directory.
CFLAGS_tests/blob_kunit.o := -I $(srctree)/$(src)/tests
CFLAGS_tests/event_kunit.o := -I $(srctree)/$(src)/tests
CFLAGS_tests/message_kunit.o := -I $(srctree)/$(src)/tests
CFLAGS_tests/metadata_kunit.o := -I $(srctree)/$(src)/tests
CFLAGS_tests/registration_kunit.o := -I $(srctree)/$(src)/tests

# The test device and the generated kernel interface of its family.
obj-$(CONFIG_CTLV_TEST) += ctlv-test.o

ctlv-test-y += tests/ctlv_test.o \
	       tests/ctlv_test_gen.o

# The same for the family inheriting it, a module of its own so that the chain
# is shown crossing one.
obj-$(CONFIG_CTLV_TEST_CHILD) += ctlv-test-child.o

ctlv-test-child-y += tests/ctlv_test_child.o \
		     tests/ctlv_test_child_gen.o
