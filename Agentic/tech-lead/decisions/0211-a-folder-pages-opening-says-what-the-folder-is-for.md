# 0211 — A folder page's opening says what the folder is for; the rest goes to a file's header
date: 2026-09-21
by: planner

## Decision
In work order 016, a `<folder>.md` whose opening (the text before its first entry) is over the
400-character cap keeps one or two sentences on what the folder is for, at 400 characters or fewer.
Every other claim in it moves by 0210's method, into a code file's header in that folder tree:

- A claim about what one file owns goes into that file's header; about one function, above it.
- A claim about the folder as a whole goes into the header of the folder's public header, the one
  other folders include (`render/include/render/device.h`, `theme/include/theme/theme.h`); a
  folder with none (`dev`) puts it in its program's `main.c` header.
- A claim a header in the tree already makes is not written a second time; the opening simply
  loses it.
- A header that gains text stays at or under the cap it had room for (55 lines if the card is on it,
  60 otherwise), tightening its own wording if it must.
- A `.md` page that sends the reader to the folder page's opening for what moved names the file.

`checks.sh --folder <f>` checks `<f>/<f>.md`; `--all` checks only folders holding code files
directly, so `dev`, `render`, `theme` and `ui` pages are caught by the folder check alone. The card
of each such folder whose proof is `--folder <f>` exits 0 takes that page's opening; earlier cards
there count it among the findings they leave untouched.

## Reasoning
The cap came after the pages were written (0208's cleanup), and 016 promises nothing true is
deleted and no new file holds prose (0210). The file headers are where the checks send documentation;
the owning file is where a reader looks for the claim. Alternatives: raise the cap for these four
pages (the cap would be a suggestion); move the prose into the subfolders' pages (their openings share
the same cap and it only moves the finding).

## Replaces
nothing
