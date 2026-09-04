# dev

The one program a person runs to see what the engine can currently do. Today
that is a window with two textured cubes in it and a camera that either orbits
them or is flown with the keyboard and the mouse. Not a menu of past states and
not a test — it is looked at, not asserted on.

- `src/main.c` — opens a window, reads its keyboard and mouse, decodes the
  texture and uploads it, draws into the window every frame, and reports what
  changes until it closes. Its header is the list of things to look at in both
  camera modes, what each of them fails like, and the list of things to try. A
  call site and nothing else; the key bindings are the only decision in it.
- `src/texture.png` — the picture on the cubes, `#embed`ded at build time the way
  the shaders are, because `platform` has no file API to read one with. An "F"
  with four differently coloured corners: it is the shape that reads wrong under
  a flip and under a mirror, which is what makes "the right way up" something a
  person can check at a glance.
