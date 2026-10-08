# 34 — The mips test reads its low-angle case without anisotropy
folder: render/tests
after: 33
decisions: 0168, 0369

## Change
Card 33 turned the SMOOTH sampler's anisotropy off (0369). Two texts in this
folder still say the low-angle case runs with it on; the test code stays.

- `render/tests/mips.c` header comment: the third case's sentence ending
  "which is anisotropic filtering with its mips and not a smear or a crawl"
  says instead that it is trilinear with its mips, anisotropy being off on
  the checked card (0369), and still not a crawl.
- `render/tests/tests.md`: the `mips.c` entry drops "with anisotropy on".

## Done when
`grep -n "anisotropy on\|anisotropic filtering with" render/tests/mips.c
render/tests/tests.md` prints nothing, and `bash
~/.claude/skills/checks/scripts/checks.sh --folder render/tests` prints
FINDINGS: 0.
