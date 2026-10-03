# 0343 — The browser starts beside the project, with its row chosen
date: 2026-10-03
by: planner

## Decision
For 053.

1. **`voe_editor_browser_show` takes a `beside` folder**, absolute or NULL. When it is given, its
   parent is listed and a row of that listing carries its name, the browser starts in the parent
   with that row chosen. When it is given but either fails (the folder was renamed or removed, or
   the parent will not list), the browser starts in the home folder, else the current one, as a
   first showing does. NULL keeps today's rule: the folder kept from the last showing, home the
   first time.
2. **Open and Save pass the project's folder**, or for an untitled scene the last project read
   from `last_project.h`, or NULL when there is none. Import passes NULL and keeps its folder.
3. **A chosen row is drawn as a choice row** (`voe_ui_choice_begin`, inverted, ADR-0194). Any
   navigation (a row entered, Up, Make folder) clears it. A press on a row still enters it.
4. **Open's Confirm acts on the chosen row while there is one**, else on the shown folder; the
   browser holds that path as `target`. Save's Confirm always saves into the shown folder: the
   chosen row there only shows where the last project is, since saving into an existing project's
   folder is refused as not empty.
5. **New still makes an untitled scene** (session.h). The browser "that asks where to put the new
   project" is that scene's first Save, which starts beside the last project by point 2.

## Reasoning
The editor already knows the project's folder and the last one opened; starting beside it costs
one parent listing. A chosen row that Confirm acts on makes Open-then-Open reopen the project with
no click, and the projects beside it stay one press away. Making New show a browser would change
what New does, which the feature does not ask for; its first Save already browses.

## Replaces
Amends browser.h's "the browser keeps its folder across showings, for the session" for Open and
Save; Import keeps it.
