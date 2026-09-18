# 10 — Live editing, and a broken theme keeps the last good one
folder: editor
decisions: 0168, 0172
read: feature.md

## Change

`src/themes.c` / `.h`: once a second by `voe_platform_clock_now`, the chosen theme's file — that file only, and
nothing for the built-in — is read again and its bytes compared with the last good ones. Changed bytes are read
and derived into a fresh arena; on success that entry's palette is replaced, set on the context, and the old
arena destroyed, so the next frame draws it. A refusal leaves the drawing palette and its arena untouched and
the check call says so by its return; `src/main.c` fills the session's notice with
`voe_editor_notice_from_report` naming the file (the reader's report names the line and what is wrong). The next
good save clears that notice. The header says why the comparison is the bytes and not a timestamp (ADR-0172)
and what a refusal leaves behind. `src/src.md` updated.

## Done when

`checks.sh --all` exits 0.

For the human, from `feature.md`'s `## How to test`, with the editor started from `./build/debug/editor/voe_editor`:
1. It opens in the near-black built-in theme; the selected Scene row is in the accent with no `> `; a pressed
   button and a dragged number show the accent.
2. Preferences in the top bar opens the list with the one in use marked; Close and Escape close it.
3. A `.theme` file dropped in `~/.config/voe3d/themes/` appears in Preferences after a restart; choosing it
   restyles every panel, and a restart keeps it.
4. `mode=light` makes it light and readable; changing only `accent` recolours the accent; the two scalars move
   surface and text separation.
5. Saving the chosen file shows the change within about a second.
6. Saving a mistake keeps the last good theme and shows a notice with the file, the line and what is wrong.
7. In Preferences each row is drawn in its own theme.
8. A theme naming `font=pixel_operator` or another `text_size` draws in it when chosen.

## Blocked
The change is made and `checks.sh --folder editor` and the whole suite (`cmake -P check.cmake`) pass, but
`checks.sh --all` still exits 1 on one finding outside this card's folder: `CLAUDE.md` line 16, heading
`## Seeing what was drawn` is not one of Checks, Exempt, Never touch. Moving that text under an allowed heading
(a tech-lead edit to `CLAUDE.md`) unblocks it; nothing in `editor` needs to change.
