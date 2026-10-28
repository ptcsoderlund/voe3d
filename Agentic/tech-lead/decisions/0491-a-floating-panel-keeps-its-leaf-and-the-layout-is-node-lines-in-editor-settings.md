# 0491 — A floating panel keeps its leaf, and the layout is node lines in `editor_settings`
date: 2026-10-28
by: planner

## Decision
For 100, carrying out 0439.

1. **The panels that move** are the closables with a header: Scene list, Assets, Inspector, Frame,
   Project, Errors and Landscape. Landscape joins `voe_editor_closable` after Frame but not the Panels
   list, which keeps its seven rows: it opens from a `.landscape` file. The two scene views are one place,
   "the views", the split of view 0 and view 1: never moved, never split apart, never floating; the
   bottom view keeps its header and × and does not drag. Preferences, the browser and the questions
   stay covers over the dock.
2. **A floating panel keeps its leaf in the tree, not laid**, as a closed one is not (0363), so a
   double-click on its header puts it back exactly. Frame, Project, Errors and Landscape are not in the
   default tree and start floating; one with no leaf docks back at its default: Frame and Landscape
   under the Inspector, Project under Assets, Errors under the views, the dock's right edge when that
   place is not laid; held at its box's size along that axis. The default tree is today's.
3. **A floating box** is kept in millimetres from the dock area's top left (below the bar) as the
   person set it, and shown clamped: at least 30 × 20 mm, at most the area, moved inside it. Defaults:
   Frame at (0, 0) 60 × 100; Project and Landscape at (60, 10) 90 × 50; Errors at (0, 60) 200 × 50.
   The last pressed or dropped is on top. Its edges are bands of ±0.5 mm (0226) that resize it, two at
   a corner.
4. **A drag starts past 1 mm** of a press on a header (0282). The drop target, first match: within 4 mm
   of the dock area's edge, that edge of the dock; over another shown floating box, floating; over a
   laid leaf that is not a view and not the panel's own, that leaf's edge nearest the pointer when within
   a quarter of the leaf's side, at most 15 mm; over the views, their edge by the same rule; else
   floating at the pointer less the grab, at its present size. Docking puts a new split where the
   target was, the panel held on the dropped side at its present size along the axis, clamped by the
   arrangement as 0226 says. The outline is the strip of the target's rectangle that deep, or the
   floating box, drawn as the lit seam's two stripes (0232) round it with nothing filled; the ghost
   shows the name (0285) and is never refused.
5. **Open flags.** The four docked flags are kept as 0363 says. Project, Errors, Frame and Landscape
   are open while their own `showing` is, copied onto the root each frame, never remembered; 0363
   point 4's one-at-a-time stands. None of the four takes the dock's pointer any more.
6. **A pointer over a shown floating box is that panel's**: no seam, view fly, middle drag, brush,
   gizmo, Assets drag or pick starts under it; `ui` keeps it.
7. **Kept in `editor_settings`** as lines: `dock_root <i>`; `dock_node <i> split <row|column>
   <fraction|first|second> <length> <fraction> <first> <second>`; `dock_node <i> leaf <panel>`, a view
   `leaf view <0|1>`; `dock_float <panel> <0|1> <x> <y> <wide> <high>`, `%.3f` mm and `%.6f` fraction;
   panel words `scene assets inspector view frame project errors landscape`. Written with the panel
   write, which runs after a drop, a dock-back, a float's move or resize and a seam drag. A tree that
   fails its check — indices under the node count, every node reached once from the root by an explicit
   stack (rule 14), each moving panel at most once and in the tree or floating, the views one split of
   view 0 and view 1, a held length in (0, 1000], a fraction in (0, 1) — means the default tree with
   0226's size keys read over it; a missing or bad float line means that panel's default. The size keys
   are still written and read only when no tree is kept.
8. A tree holds 32 nodes. A seam's double-click finds its default by the panel it holds, not by node
   index; the views' split a half.

## Reasoning
- Removing a floating panel's leaf loses where it was; a separate "last docked" record dangles once the
  tree around it changes. A leaf not laid is 0363's mechanism already.
- One flat line per node parses with no nesting, so a file cannot make the reader recurse (rule 14);
  a bracketed tree text would need a nesting limit and a parser.
- Landscape is in because the person asked for every panel with a header; out of the Panels list
  because reopening it needs a file.
- The views stay one place because 0439 says they take the room the panels leave.

## Replaces
nothing. Amends 0363 point 4: Project and Errors no longer cover the dock's pointer.
