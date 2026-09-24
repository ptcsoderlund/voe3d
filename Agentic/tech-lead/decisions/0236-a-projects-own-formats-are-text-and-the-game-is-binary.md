# 0236 — A project's own formats are text, and what the game runs is binary
date: 2026-09-24
by: tech-lead

## Decision
Every file format this engine defines for a project is **text**, so that a person or an AI can
read, compare, merge and edit it without the editor. That covers scenes, settings, and any
material, shader-setup or prefab files to come. Formats defined by an outside specification stay
as they are: PNG, glTF, WAV, fonts and the like are never converted into a text form in the
project. What the game runs is **binary**. The cook turns the text into data that is ready to
use, and it is compiled into the game program (ADR-0040, ADR-0144). The shipped game never reads
or parses a project's text format. A new format of our own that is not text needs a decision
that says why.

## Reasoning
Text lets diffs, merges and AI agents work on a project directly. Binary lets the end user load
the game fast and ship nothing extra. The cook is already the one place where the text becomes
the binary (0235), so neither side pays for the other. Rejected alternatives:
- binary project files: fast, but opaque to git and to agents.
- text shipped in the game: slower to start, and it would need a parser in the game.

## Replaces
nothing
