# 0172 — A theme is one file in a themes folder, and a program remembers which by its file name
date: 2026-09-17
by: planner

ADR-0096 made a theme an authored file in the sectioned format and left D-160 open: where such a
file lives and how a program names one. Spec 006 answers the first half for the editor —
`~/.config/voe3d/themes/` — and asks for the choice to survive a restart, for a broken file to keep
the last good theme, and for a file saved while the editor runs to show within a second.

## Decision

**One theme is one file, holding exactly one section, and the section's name is what a person
reads.** `accent`, `contrast_strength`, `surface_separation` and `mode` are required; `font` and
`text_size` are optional and fall back to the built-in theme's. A second section is refused rather
than ignored, so a file is never half-read.

**A program finds themes by listing a folder for `*.theme`.** The editor's folder is
`<settings>/voe3d/themes/`, made when it is not there; a game's lives in its project, which is the
same rule with a different folder and needs no new mechanism.

**A theme is remembered by its file name, not by its display name**, in
`<settings>/voe3d/theme` beside `last_project` — one line, empty or absent meaning the built-in
theme. A display name is the author's to change; the file name is the identity the folder already
enforces to be unique.

**The built-in theme is compiled in and has no file.** It is `ui`'s default inputs, it is what a
first start uses, and it is what a broken or missing chosen theme falls back to.

**A bad theme file changes nothing.** The reader refuses it whole, naming the line and what is
wrong through `base/report.h`; the program keeps the palette it is drawing with and puts the
refusal in front of the person, with the file's name — which the reader never sees — added by
whoever opened it.

**Live editing is the program's, not the reader's:** the editor re-reads the chosen theme's file on
a timer and compares the bytes with the last good ones. `platform` gains nothing; a theme file is
small enough that reading it once a second is free, and nothing else is watched.

## Reasoning

- **Several themes per file** — ADR-0096's sketch allowed it and nothing wants it; it makes
  *remember which theme* two identifiers instead of one and makes live editing watch a file whose
  other sections are somebody else's.
- **Remembering the display name** — two files may claim one name, and renaming a theme loses the
  choice silently.
- **A file-modification time from `platform`** — a real API addition for a comparison of a few
  hundred bytes, and a timestamp lies across a copy or a checkout.
- **Watching the whole themes folder** — the spec asks for the chosen file only, and a folder
  watch is an API `platform` does not have.
- **One settings document for the whole editor** — the right shape eventually, and it would rewrite
  `last_project`, which 004 has just accepted. The sibling file is what 005 extends, and
  consolidating them is its own decision.

## Consequences

- Three one-line files will live in `<settings>/voe3d/` once 005 lands its font override. That is
  the point at which one settings document earns itself.
- A theme that names a font the binary does not carry is refused, since the font set is an enum
  (ADR-0167) and cannot fail at a file's mercy.
- A person editing a theme sees a mistake as a notice with a line number rather than as a broken
  interface.
- Nothing here is the cooked game's: a cooked game's theme is fixed when it is cooked, and none of
  this polls anything.

## Replaces

Nothing. Written on `feature/006-themes` under the earlier workflow as 0170; renumbered 0172 by ADR-0168 when the branch took the Agentic workflow.
