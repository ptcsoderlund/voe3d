# theme

The step between a theme file and `ui` (ADR-0170): the bytes of one `.theme`
file in, `ui`'s authored theme inputs, the typeface the file named and the name a
person reads out. It opens no file and derives no palette — the caller hands it
bytes and hands what comes back to `voe_ui_theme_derive`.

- `include` — the public header, in `include/theme/`; listed below by path.
- `src` — the implementation; its file is listed on `src/src.md`.
- `tests` — one plain C program, found by the build; listed on `tests/tests.md`.
- `include/theme/theme.h` — `voe_theme` and `voe_theme_read`. Its header says
  the file's shape, every refusal and the line each names, and that nothing
  here names `render`; `voe_theme.name` says why the identity is the file name
  and not the section's (ADR-0172).
