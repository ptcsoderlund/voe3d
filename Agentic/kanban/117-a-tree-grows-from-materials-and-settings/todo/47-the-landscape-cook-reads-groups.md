# 47 — The landscape cook reads groups
folder: authoring
after: 45
decisions: 0168, 0521

## Change
Card 45 renamed a landscape copy's `kind` to `group` and the landscape's kinds to groups.
`authoring/src/landscape_cook.c`, `authoring/include/authoring/landscape_cook.h` (if it names them) and
`authoring/tests/landscape_cook.c` use the new names. The cooked text is unchanged: a copy's first number
is its group. Header points naming a kind say group.

## Done when
`cmake --build --preset debug --target voe_authoring` exits 0, and
`ctest --test-dir build/debug -R '^authoring/landscape_cook'` exits 0 after building its test target.
