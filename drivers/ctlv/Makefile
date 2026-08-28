# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_CTLV) += ctlv.o

ctlv-y += blob.o blob_desc.o builder.o core.o event.o introspection.o \
	  message.o schema.o ctlv_gen.o

# The suites are a separate module. The fixture families exercise the schema
# tool, and their generated metadata is walked beside the root family's.
obj-$(CONFIG_CTLV_KUNIT_TEST) += ctlv-kunit.o

ctlv-kunit-y += tests/ctlv_kunit_mod.o \
		tests/blob_kunit.o \
		tests/event_kunit.o \
		tests/message_kunit.o \
		tests/metadata_kunit.o \
		tests/registration_kunit.o \
		tests/ctlv_fixture_gen.o \
		tests/ctlv_fixture_child_gen.o

# Use the fixture registry before the installed UAPI registry.
ctlv-kunit-cflags := -include $(src)/tests/linux/ctlv_families_gen.h \
		     -I $(src)/tests
CFLAGS_tests/blob_kunit.o := $(ctlv-kunit-cflags)
CFLAGS_tests/event_kunit.o := $(ctlv-kunit-cflags)
CFLAGS_tests/message_kunit.o := $(ctlv-kunit-cflags)
CFLAGS_tests/metadata_kunit.o := $(ctlv-kunit-cflags)
CFLAGS_tests/registration_kunit.o := $(ctlv-kunit-cflags)
CFLAGS_tests/ctlv_fixture_gen.o := $(ctlv-kunit-cflags)
CFLAGS_tests/ctlv_fixture_child_gen.o := $(ctlv-kunit-cflags)

# The test device and the generated kernel interface of its family.
obj-$(CONFIG_CTLV_TEST) += ctlv-test.o

ctlv-test-y += tests/ctlv_test.o \
	       tests/ctlv_test_gen.o

# The same for the family inheriting it, a module of its own so that the chain
# is shown crossing one.
obj-$(CONFIG_CTLV_TEST_CHILD) += ctlv-test-child.o

ctlv-test-child-y += tests/ctlv_test_child.o \
		     tests/ctlv_test_child_gen.o
