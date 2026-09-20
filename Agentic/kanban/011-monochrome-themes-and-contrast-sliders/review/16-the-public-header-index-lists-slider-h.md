# 16 — The public header index lists slider.h
folder: ui
decisions: 0168, 0196

## Change
`ui/include/ui/ui.md` is the index of that folder's public headers and is the only file this card
touches. Card 11 added `ui/include/ui/slider.h` and listed it on `ui/ui.md` but not here, which is why
`checks.sh --all` reports `ui/include/ui/ui.md: does not list \`slider.h\``.

- Add one entry, between the `layout.h` one and the `theme.h` one so the list stays alphabetical, in
  exactly the shape the other entries have — a line opening `` - `slider.h` — `` and a sentence after
  it. The name is the bare file name, not a path: the check looks for a file of that name beside the
  `.md`. The sentence: the slider — a track of a given width in millimetres with a thumb at the value's
  place in a range — which IS a number box, so it is dragged, typed into and drawn inverted while held,
  and what it costs in nodes and element records. Take the wording from the `include/ui/slider.h`
  paragraph of `ui/ui.md`, shortened to one sentence the way the other four entries here are short.
- The opening sentence of the file says the fuller account of "all four" stays on `ui/ui.md`; with this
  entry there are five headers, so say five.

Read `ui/ui.md`'s `include/ui/slider.h` paragraph for the wording and nothing else — no header, no
source file. Keep the file to its one `# ui` heading, with no code block and no table; that is what
the check allows.

## Done when
`checks.sh --folder ui` prints `FINDINGS: 0` and exits 0 — the finding above is gone and no new one
has taken its place.
