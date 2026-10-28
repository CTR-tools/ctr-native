# Upstream provenance

- Project: <https://github.com/mkst/maspsx>
- Commit: `42b862c988fe7a13fe4e7ac0ebec90ed6b9fb763`
- License: MIT (`LICENSE`)

This directory is a source snapshot with the upstream `.git` directory
removed. The matching pipeline executes `maspsx.py` directly.

Local change: model MFC2/CFC2 CPU load delays in reorder regions, including
register aliases and conservative gaps for unrecognized instructions. Reuse
the existing `$at`/`$gp` expansion policy; explicit noreorder remains literal.
The focused cases are in `tests/test_cop2.py`.

Local change: preserve ordering controls consumed by MFLO/MFHI hazard
lookahead; defer load/transfer prefixes without duplicate labels or early
source-mode changes. Regression cases are in `tests/test_mflo.py`.

Pinned executable source:

```text
d170463b0c519a825db97d3f30ffbe0228e4cf642de2dc64fb30f90f118c2b0c  maspsx.py
7b22e9a7dfe3c620f4cfff6c785da774cf927aff57e012ad414baa335be7bbf5  maspsx/__init__.py
```
