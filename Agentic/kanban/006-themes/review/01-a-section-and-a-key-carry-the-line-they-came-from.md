# 01 — A section and a key carry the line they came from
folder: assets
decisions: 0168

## Change

Done on 2026-09-17 under the earlier workflow as task 1 of spec 006; converted to a card by ADR-0168.

Add `uint32_t line` to `voe_assets_sectioned_section` and to
`voe_assets_sectioned_key`, the 1-based physical line of the `[Section]` header and of the
`key=value` line, filled by the parser as it walks (it already counts lines for its refusals).
Say in the header that a consumer naming a line in its own refusal takes it from here and does
not walk the text again. Add the cases to `tests/sectioned.c`: lines across comments, blank
lines, `\r\n` endings and a file whose first section is not on line 1. Nothing else about the
parser changes.

## Done when

`cmake --build --preset debug --target voe_assets && ctest --test-dir build/debug -R '^assets/'` — all tests pass.
