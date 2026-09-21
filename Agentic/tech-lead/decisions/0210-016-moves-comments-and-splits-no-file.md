# 0210 — 016 moves comments, splits no file, and proves it by tokens
date: 2026-09-21
by: planner

## Decision
Work order 016 changes comments and folder `.md` pages and nothing else. A file over 800 lines is not
split first: splitting moves code, and 016 moves none. Every 016 card follows one method.

- A header comment is the leading comment; `checks.sh` counts a line as ceil(length / 100), and a blank
  line ends the header. Target 55 lines or fewer, counted that way.
- The header keeps what the file does, how it is used (a short usage sketch may stay), its constraints,
  and whatever holds for the whole file. Rule 14's nesting-limit reason stays in the header (0168).
- A paragraph about one function, type, constant or field moves directly above that declaration or
  definition, word for word where it still holds; one about several goes above the first of them. In a
  header and its source both, it goes where a reader of that function looks first: the declaration in a
  public header, the definition in `src/`. A claim no longer true is dropped; nothing true is.
- Nothing is parked below the header behind a blank line, and no new file is made to hold prose.
- `ADR-NNNN`, `card NNN`, `rule N` and `spec NNN` citations travel with their sentences.
- A `<folder>.md` entry that sends the reader to "its header" for something that moved says the file
  instead; every entry stays at 300 characters or fewer.

The proof that no code moved, run from the repository root before the commit, prints nothing:

    t(){ clang -Xclang -dump-raw-tokens -fsyntax-only -x c "$1" 2>&1 | grep -E "^[a-z_]+ '" | grep -vE '^(comment|unknown) ' | cut -f1; }
    for f in $(git diff --name-only HEAD -- '*.c' '*.h'); do cmp -s <(git show "HEAD:$f" | t -) <(t "$f") || echo "code changed: $f"; done

## Reasoning
The work order promises nothing the programs do changes; a token comparison proves that where a build
cannot, since `__LINE__` in the asserts moves with the comments and `window_win32.c` does not build on
Linux. Alternatives: split the files over 800 lines first (code moves in a comment-only feature, a card
per file, and several are public headers); leave the method to each card (the same thirty lines on
twenty-three cards).

## Replaces
nothing
