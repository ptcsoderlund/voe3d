# theme

The step between a theme file and `ui` (ADR-0170): the bytes of one `.theme`
file in, `ui`'s authored theme inputs, the typeface the file named and the name a
person reads out. It opens no file and derives no palette — the caller hands it
bytes and hands what comes back to `voe_ui_theme_derive`. `render` is on its row
only for the test's headless device (ADR-0176); `src/` and `include/` name no
`render/` header.

- `include` — the public header, in `include/theme/`; listed below by path.
- `src` — the implementation; its file is listed on `src/src.md`.
- `tests` — one plain C program, found by the build; listed on `tests/tests.md`.
- `include/theme/theme.h` — `voe_theme` and `voe_theme_read`. Its header says
  the file's shape with an example, every refusal and the line each names, why
  the name a person reads is the section's while the identity is the file name
  (ADR-0172), and that nothing here names `render`.
