# 0351 — A panel can be closed and reopened from the Panels menu
date: 2026-10-04
by: tech-lead

## Decision
Every editor panel except the top bar and the top scene view can be closed: the Project panel, Scene list, Assets,
Inspector, Errors and the bottom scene view. A panel closes from an × in its header, or from a **Panels** menu in
the top bar that lists every closable panel with a tick beside each open one; clicking an entry opens or closes
that panel. A closed panel gives its space to its neighbour. A reopened panel comes back in the same place, at the
size it had when it closed. Which panels are open is kept with the panel sizes in the global editor settings file
(0220, 0226); a missing entry means open. The Errors panel opens itself whenever a build fails, even if it was
closed. Whether a panel is open is its own state, kept apart from where it sits and how big it is, so a later
docking feature can add moving panels without changing how closing works. There is no minimize or collapse
button: closing is the only way to hide a panel.

## Reasoning
The sponsor's call (2026-10-04): closing comes first, docking later if a real layout needs it. Keeping "open" apart
from "where" means nothing built now has to be undone for docking. The menu is called Panels, not View, because
the editor already has scene views. Rejected: docking now, several times the work with no need yet; a +/−
minimize per panel, a second way to hide things that docking would have to support too; a closed Errors panel
staying shut on a failed build, which makes a failed Play look like nothing happened.

## Replaces
nothing. Extends 0220.
