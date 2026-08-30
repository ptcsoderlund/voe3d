# dev

The one program a person runs to see what the engine can currently do. Today
that is a window the GPU has filled with a colour; later a triangle. Not a menu
of past states and not a test — it is looked at, not asserted on.

- `src/main.c` — opens a window, clears it every frame, and reports its size
  until it closes. A call site and nothing else; anything in it worth keeping
  belongs in a folder.
