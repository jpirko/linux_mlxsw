# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The single error type reported by the CTLV schema tool."""


class CtlvError(Exception):
    """A specification, registry or ABI snapshot is not acceptable.

    The test suite matches diagnostics, so a message's wording is contract.
    """

    def __init__(self, where, what):
        self.where = where
        self.what = what
        super().__init__(f'{where}: {what}' if where else what)
