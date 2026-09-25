# 0247 — Every string is UTF-8 and no character makes anything fail
date: 2026-09-25
by: tech-lead

## Decision
Every string the engine, editor and a shipped game hold or pass is UTF-8, on both platforms, with
no exceptions: text, names, window titles, typed input, and every path, including those that come
from the operating system (home, settings, the program's own folder, a project folder). No
character in any script can crash, corrupt a file, or make an open, save, load or list fail or hit
the wrong file. For example: a Windows user named `Åsa` or `李`, a game installed under
`C:\Spel\Hörnet\`, or a project folder `Min värld`. On Windows the engine talks to the system in
UTF-8 or UTF-16 and never in the legacy code page, so no Windows "A" function receives a path or
name. Malformed UTF-8 is replaced by U+FFFD and never asserts in a shipped game. A character the
font does not carry is drawn as the missing-glyph box. This decision does not require it to be
drawn as a letter; covering more scripts (fallback fonts, shaping) needs its own decision. The
planner picks the Windows mechanism (a UTF-8 code-page manifest or the "W" functions).

## Reasoning
A developer's game must not break at a customer's machine because of a name or path. Today the
Windows file, folder, path and environment calls use the "A" functions with no UTF-8 manifest, so a
home folder outside the system code page fails and one inside it arrives in non-UTF-8 bytes.
Alternatives: ASCII-only paths (breaks for every Swedish user folder, rejected); drawing every
script now (a font and shaping feature far larger than the risk, deferred).

## Replaces
Nothing. It overrides the "a path here is a name spelled in ASCII" premise in `platform`'s Windows
files.
