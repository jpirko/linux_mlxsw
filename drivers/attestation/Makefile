# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_ATTESTATION) += attestation.o

attestation-y := core/core.o core/ctlv.o core/events.o core/mr.o \
		 generated/ctlv_attestation_gen.o

obj-$(CONFIG_ATTESTATION_TDX) += attestation-tdx.o

attestation-tdx-y := providers/tdx.o \
		     generated/ctlv_attestation_tdx_gen.o
