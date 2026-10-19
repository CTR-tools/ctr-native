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

Pinned executable source:

```text
d170463b0c519a825db97d3f30ffbe0228e4cf642de2dc64fb30f90f118c2b0c  maspsx.py
fb5eccb7d6570a5ffc23de4e289a3f88199f30d46c0677aee4acda37f81d0c4e  maspsx/__init__.py
```
