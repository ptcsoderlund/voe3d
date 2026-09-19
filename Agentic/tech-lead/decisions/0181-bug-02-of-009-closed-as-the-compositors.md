# 0181 — Bug 02 of 009 is closed as the compositor's
date: 2026-09-19
by: tech-lead

## Decision
Bug 02 of 009 ("the editor swallows the desktop's keyboard") is closed with no engine change. After a
reboot the sponsor cannot reproduce it: with the editor open, the start menu and other windows on the
same virtual desktop take the keyboard as intended. The fault is taken to have been a transient state in
the Wayland session (KWin), not the engine. The bug report is removed, and card 04
(`blocked/04-the-editor-lets-go-of-the-desktops-keyboard.md`) is withdrawn and not re-planned. 009 is
judged on its own `## How to test` steps. If the keyboard is ever held again, it is filed as a new bug
on its own feature, and the four key tests from 009's needs-decision (vkcube, voe_dev, the editor at
HEAD, the editor before card 03) are its first step.

## Reasoning
The problem went away after a reboot, and card 04 had already found nothing in the engine that could
hold a keyboard. Alternatives: running the four key tests anyway, which is pointless now that the bug
can't be reproduced; moving the bug to its own feature, which would leave a feature with nothing to fix.

## Replaces
nothing
