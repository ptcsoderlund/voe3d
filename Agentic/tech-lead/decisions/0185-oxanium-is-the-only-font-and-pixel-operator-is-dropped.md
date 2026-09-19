# 0185 — Oxanium is the only font; Pixel Operator and the editor's font override are dropped
date: 2026-09-19
by: tech-lead

## Decision
The engine carries one font, Oxanium. Pixel Operator leaves the tree: the embedded face, its
licence file and every name for it. Both of the editor's built-in themes, Near black and Near
white, use Oxanium again, as ADR-0178 first said. The editor offers no font choice. The font row in
Preferences and the remembered `<settings>/voe3d/font` line are removed, and a leftover file of
that name is ignored. Oxanium is also the fallback: a theme whose `font=` names anything else,
`pixel_operator` included, draws in Oxanium and reports nothing. The glyph cutoff work of bug 03
(ADR-0182, ADR-0183, ADR-0184) stands.

## Reasoning
The sponsor re-ran the editor after bug 03's fix and found Oxanium "phenomenal, even on smaller
sizes". With one font there is nothing to choose, and the cheapest feature is the one not kept.
Alternatives rejected: keep Pixel Operator as an option with Oxanium as the default, which leaves
a face, a licence and a Preferences row that nobody uses; keep the override machinery for future
fonts, which can be rebuilt when a second font is actually wanted.

## Replaces
ADR-0179 as a whole. ADR-0167's second embedded face: Pixel Operator is removed.
