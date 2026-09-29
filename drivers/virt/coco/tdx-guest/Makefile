# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_TDX_GUEST_DRIVER) += tdx-guest.o

tdx-guest-y := main.o
tdx-guest-$(CONFIG_ATTESTATION) += attestation.o ctlv_attestation_tdx_gen.o

# The generated family header includes its parent's.
ccflags-y += -I $(srctree)/drivers/attestation/generated
