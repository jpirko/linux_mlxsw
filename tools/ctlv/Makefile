# SPDX-License-Identifier: GPL-2.0

KDIR = ../..
DOCDIR = $(KDIR)/Documentation/ctlv
SPECDIR = $(DOCDIR)/specs
SPECS = $(wildcard $(SPECDIR)/*.yaml)
TOOL = ./ctlvgen/cli.py

all: validate abi-check

validate:
	@for spec in $(SPECS) ; do \
		echo -e "\tVALIDATE $$(basename $$spec)" ; \
		$(TOOL) --spec $$spec --validate || exit 1 ; \
	done

# Fails when a specification no longer matches its checked-in ABI snapshot.
abi-check:
	@for spec in $(SPECS) ; do \
		echo -e "\tABICHECK $$(basename $$spec)" ; \
		$(TOOL) --spec $$spec --abi-check || exit 1 ; \
	done

# Deliberate: an updated snapshot is how a UAPI change gets reviewed.
abi-update:
	@for spec in $(SPECS) ; do \
		$(TOOL) --spec $$spec --abi-update || exit 1 ; \
	done

# Style checks, needing yamllint, pycodestyle and pyflakes, which the tool
# itself does not use.
#
# A flow sequence is written "[ one, two ]" here, as in the tree's other
# meta-schemas, which stock yamllint rejects; the same relaxation is in
# Documentation/devicetree/bindings/.yamllint.
YAMLLINT_CONF = {extends: default, rules: {brackets: {max-spaces-inside: 1}}}

PYSRC = ctlvgen

lint:
	yamllint -d "$(YAMLLINT_CONF)" $(DOCDIR)
	python3 -m pycodestyle $(PYSRC)
	python3 -m pyflakes $(PYSRC)

# The C half of this directory: the userspace library, and the tool that drives
# a device it was never compiled against. Neither is generated and neither is
# Python, so they are their own target rather than part of the checks above.
programs:
	$(MAKE) -C libctlv
	$(MAKE) -C ctlvctl

regen:
	@./ctlv-regen.sh

# Fails when a checked-in generated file does not match what the generator
# produces from the current specifications. Writes nothing.
check-regen:
	@./ctlv-regen.sh -c

clean:
	rm -rf ctlvgen/__pycache__
	rm -rf ctlvgen/lib/__pycache__
	rm -rf ctlvgen.egg-info
	rm -rf build
	$(MAKE) -C libctlv clean
	$(MAKE) -C ctlvctl clean

.PHONY: all validate abi-check abi-update lint programs regen check-regen clean
