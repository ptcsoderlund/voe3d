# 01 — The records' layout asserts leave descriptors.c
folder: render
after: none
decisions: 0168

## Change
`render/src/descriptors.c` is 791 lines, and cards 06 and 10 grow the records it asserts on. This
card splits it by what each part does. Nothing changes behaviour. Read the header of
`render/src/descriptors.c` and its block of `static_assert`s, from the comment "Every record a
shader reads by index…" through the element record's asserts (about lines 67–161).

- `render/src/records_layout.c` (new): the block moves here unchanged, with the includes it needs
  (`device_internal.h`, `<stddef.h>`). It has no functions. Header points:
  - what the file proves: the C and Slang layouts of every record a shader reads agree, so a
    dropped pad fails the build;
  - who adds to it: whoever changes a record in `device.h`, `device_parts.h` or a `.slang(h)`
    file;
  - it holds no code.
- `render/src/descriptors.c`: the block is gone, and so is any include that only the block used.
  The header says where the asserts went, in one line.
- `render/src/src.md`: a `records_layout.c` entry under 300 characters. The `descriptors.c` entry
  stays as it is.

The build picks the new file up by its folder (`cmake/voe.cmake`); no CMake edit.

## Done when
`[ $(wc -l < render/src/descriptors.c) -lt 720 ] && grep -c static_assert render/src/records_layout.c`
prints 40, and the tests `render/passes` and `render/shadow` pass after the folder's
build.
