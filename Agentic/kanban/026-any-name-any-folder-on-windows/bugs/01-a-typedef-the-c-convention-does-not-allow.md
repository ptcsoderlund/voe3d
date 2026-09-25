# 01 — A typedef the C convention does not allow

## Seen
Card 05 fixed a `**` finding in `platform/src/arguments_win32.c` by adding a file-local
`typedef const char *utf8_argument;`. The C convention allows `typedef` only for opaque handles
and function pointers, so the fix breaks the convention. The coder saw the conflict and followed
the card.

## Expected
`arguments_win32.c` passes the `**` check without that typedef. Every `typedef` in the file is
one the C convention allows, and `checks.sh --all` still prints `FINDINGS: 0`. On Linux, the
editor still opens a project in a folder named `Åsa 李 värld`.

## How to reproduce
1. Open `platform/src/arguments_win32.c` and find `typedef const char *utf8_argument;`.
2. Compare it with the `typedef` rule in the C convention.
