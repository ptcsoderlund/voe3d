# dev

The one program a person runs to see what the engine can currently do. Today
that is a window with a triangle in it. Not a menu of past states and not a
test — it is looked at, not asserted on.

- `src/main.c` — opens a window, draws into it every frame, and reports its size
  until it closes. Its header is the list of things to try. A call site and
  nothing else; anything in it worth keeping belongs in a folder.
