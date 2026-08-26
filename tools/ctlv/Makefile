# SPDX-License-Identifier: GPL-2.0

KDIR = ../..
DOCDIR = $(KDIR)/Documentation/ctlv
SPECDIR = $(DOCDIR)/specs
SPECS = $(wildcard $(SPECDIR)/*.yaml)
TOOL = ./ctlvgen/cli.py
DOC_OUTPUT ?= $(KDIR)/Documentation/output/ctlv/families

# Writes nothing. Fails on an invalid specification, and on any generated file
# or ABI snapshot that regen would change.
all:
	@./ctlv-regen.sh -c

# Rewrites every generated file and ABI snapshot, and names each one it writes.
# A specification change that breaks the committed ABI is refused unless
# ABI_BREAK=1 says it is meant.
regen:
	@./ctlv-regen.sh $(if $(filter 1,$(ABI_BREAK)),-b)

# Style and type checks, needing yamllint, pycodestyle, pyflakes, and mypy
# with types-PyYAML and types-jsonschema, which the tool itself does not use.
#
# A flow sequence is written "[ one, two ]" here, as in the tree's other
# meta-schemas, which stock yamllint rejects; the same relaxation is in
# Documentation/devicetree/bindings/.yamllint. bad-specs/ is left out on
# purpose: a corpus of deliberately invalid input cannot be lint-clean, since
# one of its cases is a duplicate YAML key.
YAMLLINT_CONF = {extends: default, rules: {brackets: {max-spaces-inside: 1}}}

PYSRC = ctlvgen tests

lint:
	yamllint -d "$(YAMLLINT_CONF)" $(DOCDIR) tests/specs \
		tests/families.yaml tests/generated-files.yaml
	python3 -m pycodestyle $(PYSRC)
	python3 -m pyflakes $(PYSRC)
	python3 -m mypy $(PYSRC)

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

.PHONY: all regen lint docs programs run_tests clean
