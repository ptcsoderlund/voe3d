# 06 — `authoring` takes its line numbers from the parser
folder: authoring
decisions: 0168

## Change

Card 01 put `line` on every `voe_assets_sectioned_section` and `voe_assets_sectioned_key`
(`assets/include/assets/sectioned.h`), so `authoring`'s second walk for line numbers has no reason left.

- `src/line_index.h` / `src/line_index.c` become `src/key_span.h` / `src/key_span.c`:
  `voe_authoring_key_spans(text, size, doc, out_key_span)` fills only each key's trimmed bytes (the
  `voe_authoring_span` type stays), which a kept section is written back from. `out_section_line`,
  `out_key_line` and the walk's line counting as an output go; the count-matching assert stays. Its header says
  a line is the parser's and a span is this file's, because the parser hands back values, not source bytes.
- `src/project.c` takes `doc.sections[s].line` and `doc.keys[i].line` directly and no longer calls the walk at
  all (it passed NULL for spans).
- `src/scene_read.c`: `section_line` and `key_line` leave the reader struct; every refusal reads the line off the
  parsed section or key. Its header's paragraph about walking the text beside the sectioned reader now says the
  walk is for key spans only.
- `src/src.md` lists `key_span.h` / `key_span.c` in place of the two `line_index` entries.

No refusal changes its wording and no test's expected line changes; `tests/` is not edited.

## Done when

`cmake --preset debug && cmake --build --preset debug --target voe_authoring && ctest --test-dir build/debug -R '^authoring/'`
passes with `authoring/tests/` unchanged (`git diff --quiet HEAD -- authoring/tests` exits 0), and
`grep -rn line_index authoring` finds nothing.
