# SPDX-License-Identifier: GPL-2.0

KDIR = ../..
DOCDIR = $(KDIR)/Documentation/ctlv
SPECDIR = $(DOCDIR)/specs
SPECS = $(wildcard $(SPECDIR)/*.yaml)
TOOL = ./ctlvgen/cli.py
TREE_ARGS = --spec $(SPECDIR)/ctlv.yaml

# Specifications that exist only to exercise the tool. They are not registered
# families, so they keep their registry beside themselves and only have to be
# told where the meta-schema is. Having no ABI, they have no snapshot either.
FIXTURE_SPECDIR = tests/specs
FIXTURE_SPECS = $(wildcard $(FIXTURE_SPECDIR)/*.yaml)
FIXTURE_ARGS = --schema $(DOCDIR)/ctlv-schema.yaml
DOC_OUTPUT ?= $(KDIR)/Documentation/output/ctlv/families

all: validate abi-check check-regen

tree-check:
	@$(TOOL) $(TREE_ARGS) --tree-check

validate: tree-check
	@for spec in $(SPECS) ; do \
		echo -e "\tVALIDATE $$(basename $$spec)" ; \
		$(TOOL) --spec $$spec --validate || exit 1 ; \
	done
	@for spec in $(FIXTURE_SPECS) ; do \
		echo -e "\tVALIDATE $$(basename $$spec) (fixture)" ; \
		$(TOOL) --spec $$spec $(FIXTURE_ARGS) --validate || exit 1 ; \
	done

# Fails when a specification no longer matches its checked-in ABI snapshot.
abi-check: tree-check
	@for spec in $(SPECS) ; do \
		echo -e "\tABICHECK $$(basename $$spec)" ; \
		$(TOOL) --spec $$spec --abi-check || exit 1 ; \
	done

# Deliberate: an updated snapshot is how a UAPI change gets reviewed.
abi-update:
	@$(TOOL) $(TREE_ARGS) --tree-check-update
	@for spec in $(SPECS) ; do \
		$(TOOL) --spec $$spec --abi-update || exit 1 ; \
	done
	@$(TOOL) $(TREE_ARGS) --registry-abi-update

# Style checks, needing yamllint, pycodestyle and pyflakes, which the tool
# itself does not use.
#
# A flow sequence is written "[ one, two ]" here, as in the tree's other
# meta-schemas, which stock yamllint rejects; the same relaxation is in
# Documentation/devicetree/bindings/.yamllint. bad-specs/ is left out on
# purpose: a corpus of deliberately invalid input cannot be lint-clean, since
# one of its cases is a duplicate YAML key.
YAMLLINT_CONF = {extends: default, rules: {brackets: {max-spaces-inside: 1}}}

PYSRC = ctlvgen tests

lint:
	yamllint -d "$(YAMLLINT_CONF)" $(DOCDIR) $(FIXTURE_SPECDIR) \
		tests/families.yaml tests/generated-files.yaml
	python3 -m pycodestyle $(PYSRC)
	python3 -m pyflakes $(PYSRC)

docs:
	@mkdir -p $(DOC_OUTPUT)
	@for spec in $(SPECS) ; do \
		name=$$(basename $$spec .yaml) ; \
		echo -e "\tDOC $$name.rst" ; \
		$(TOOL) --spec $$spec --mode doc --rst \
			-o $(DOC_OUTPUT)/$$name.rst || exit 1 ; \
	done

# The C half of this directory: the userspace library, and the tool that drives
# a device it was never compiled against. Neither is generated and neither is
# Python, so they are their own target rather than part of the checks above.
programs:
	$(MAKE) -C libctlv
	$(MAKE) -C ctlvctl

# What one operation costs, against the floor of the same entry point and
# against netlink. Its own target: it measures rather than ships, it needs a
# device of the test family to measure against, and nothing else here needs it
# built to be checked.
bench:
	$(MAKE) -C ctlvbench

regen:
	@./ctlv-regen.sh

# Fails when a checked-in generated file does not match what the generator
# produces from the current specifications. Writes nothing.
check-regen:
	@./ctlv-regen.sh -c

run_tests:
	@python3 -m unittest discover -s tests -t . -v

clean:
	rm -rf ctlvgen/__pycache__
	rm -rf ctlvgen/lib/__pycache__
	rm -rf tests/__pycache__
	rm -rf ctlvgen.egg-info
	rm -rf build
	rm -rf $(DOC_OUTPUT)
	$(MAKE) -C libctlv clean
	$(MAKE) -C ctlvctl clean
	$(MAKE) -C ctlvbench clean

.PHONY: all tree-check validate abi-check abi-update lint docs programs bench \
	regen check-regen run_tests clean
