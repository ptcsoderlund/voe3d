# 0363 — A closed panel is a flag beside the dock tree, and the Panels menu ticks with √
date: 2026-10-05
by: planner

## Decision
For 059, carrying out 0351.

1. Whether a dock panel is open is one flag per closable leaf on the dock root, beside the tree and not in it.
   The closable leaves are the Scene list, Assets, the Inspector and the bottom view (view 1's leaf); the top
   view never closes. A closed leaf is not laid out. A split with one side closed gives the other side its
   whole rectangle and has no seam to drag; a split with both sides closed is itself closed. Held lengths and
   the views' share are never written by a close, so a reopened panel comes back where it was, at its size.
2. Every closable dock leaf has a header strip above its content, the bottom view's above its picture: the
   panel's name at the left and an × (U+00D7) button at the right. The Errors and Project panels get a title
   row with the same ×, which replaces their Close button.
3. The top bar's **Panels** button, after Preferences, opens a list hanging below it: Scene list, Assets,
   Inspector, Bottom view, Project, Errors. An open panel's row is ticked with √ (U+221A), because Oxanium
   carries no ✓; the tick sits in a column of its own so the names line up. A click opens or closes that panel
   and closes the list; Escape or a press outside it closes it too. While it is open, the borders, the picks,
   the gizmo and drops ignore the pointer, as for the other panels over the dock.
4. Project and Errors stay panels over the dock, one at a time with Preferences: opening one from the menu hides
   the other two. Errors opened from the menu reads `Build/build.log` as a failed build does. Neither is
   remembered: both start closed each run, as they do today.
5. The four dock flags are kept in `editor_settings` as `scene_open`, `assets_open`, `inspector_open` and
   `view_open`, each `0` or `1`; a missing line or any other value means open. They are written whenever a
   panel opens or closes, and with the sizes whenever those are written.

## Reasoning
A flag beside the tree is 0351's "open apart from placement": a later docking feature moves leaves without
touching it. Not remembering Project and Errors: 0351's "a missing entry means open" would cover the dock with
both on a first start, and their content belongs to one moment (a build's log, a dialogue), not to a layout.
√ and not a drawn mark: the font has it and a label costs nothing new. Rejected: removing a closed leaf from the
tree, which loses its place and size; a tick drawn as a choice's selected state, which reads as "chosen", not
"open".

## Replaces
nothing. Amends 0351: Project and Errors are not kept in the settings file.
