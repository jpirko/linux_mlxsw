# SPDX-License-Identifier: GPL-2.0

KDIR = ../..
DOCDIR = $(KDIR)/Documentation/ctlv

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
# Documentation/devicetree/bindings/.yamllint.
YAMLLINT_CONF = {extends: default, rules: {brackets: {max-spaces-inside: 1}}}

PYSRC = ctlvgen

lint:
	yamllint -d "$(YAMLLINT_CONF)" $(DOCDIR)
	python3 -m pycodestyle $(PYSRC)
	python3 -m pyflakes $(PYSRC)
	python3 -m mypy $(PYSRC)

clean:
	rm -rf ctlvgen/__pycache__
	rm -rf ctlvgen/lib/__pycache__
	rm -rf ctlvgen.egg-info
	rm -rf build

.PHONY: all regen lint clean
