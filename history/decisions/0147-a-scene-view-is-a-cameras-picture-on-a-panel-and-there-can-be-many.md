# 0147. A scene view is a camera's picture on a panel, and there can be many

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-249**, opened by ADR-0141: *what a scene view is, in an editor whose interface
is geometry.* It is the scene-view half of the arc the principal chose on 2026-09-13 —
project, save and open, with a scene view alongside (ADR-0146).

The tech lead recommended the hole — the world drawn in the frame with the real camera, the
dock tree leaving one region uncovered — as nearly free and not blocking a picture later.
The principal answered with the requirement that settles it:

> *"Just like blender we want to be able to have multiple scene views. We might want to
> display baking information in one scene view and render result in another as an example.
> I dont think that can be done with a hole?"*

Constraints already fixed. ADR-0051: the frame is drawn into an offscreen target and copied
to the window, so a picture-then-present step exists already. ADR-0141: the interface is
world geometry at millimetre scale and its camera is replaceable; point 7 says a headset
user writes their own scene view. ADR-0142: the panel layout is a tree of data and a panel
is a value in an enumeration. ADR-0121: `render` gains nothing *for the editor's sake* —
so whatever `render` gains here must be a capability any program can use, which a second
view is (a security camera, a mirror, a minimap, a portal). And today: `render` begins a
frame with exactly **one camera and one sun**, and an element record has two kinds, a solid
and a glyph; nothing samples an arbitrary texture on the element path.

## Options considered

### Option A — a hole in the panel layout
The world drawn once, in the frame, with the editor's camera; the dock tree leaves a region
uncovered and the projection is shifted to centre on it. One card. **Several holes are
possible** — a viewport and scissor per region, the world drawn once into each — so the
principal's question does not quite die on *several*. It dies on what each view contains
and where it stands: every hole is a rectangle of the one frame target, at the window's
resolution, with the window's post-processing, and cannot exist on a panel that is not
flat against the camera — which under ADR-0141 is a panel standing in the world, or any
panel in a headset. And nothing can be drawn over a hole without knowing it is one.

### Option B — a camera's picture, drawn into its own target and shown on a panel
A view is a camera, a sun, and a colour and depth target of its own at its own size.
`render` draws it, and a panel shows its colour image as a texture. Several views are
several targets. Each may be drawn differently — the final picture in one, a debug channel
in another — because each is its own pass. What it costs: `render` learns to hold more than
one view per frame and to hand a view's image to the element path, which is new; several
cards across `render` and `editor` rather than one.

## Decision

**Option B**, the principal's requirement. The tech lead's hole was the cheaper first step
toward the wrong place.

1. **A scene view is a camera's picture on a panel.** A view is drawn into a target of its
   own, and a panel shows that target's colour image. The editor draws no world into the
   frame's own target.
2. **There can be several, and each is independent.** Its own camera, its own size, and —
   when there is more than one way to draw — its own way of drawing. **Blender's viewports**
   are the model, and the principal's two examples are the test: baking information in one,
   the render result in another.
3. **This is a `render` capability, not an editor one.** A frame that draws more than one
   view into more than one target, and a view's image usable as a texture, are things a
   mirror, a security camera or a minimap need too. That is what lets `render` gain them
   under ADR-0121 point 6. **The shape of that API is not decided here** (D-257), and it
   is the next decision.
4. **A view's size is its panel's size in pixels**, which the editor computes from the
   panel's millimetres and the surface's pixels per millimetre — the same one conversion
   ADR-0141 point 4 keeps in `editor/src/main.c`. A resized panel is a resized target.
5. **The camera a person moves is the editor's, not the scene's.** It is state in `editor/`,
   not an entity, so it is never authored and never saved (ADR-0125: identity means
   authored). Where the developer happened to be looking stays out of the scene file.
6. **A scene view is a panel in the layout** like the Scene list and the Inspector — a
   value in ADR-0142's enumeration — and several scene views are several leaves of the
   tree, each naming which view it shows.

## Blast radius

**Moderate.** The cost is paid in `render`'s public surface, which gains views and a way to
sample one; every caller of `voe_render_frame_begin` — `dev`, `editor`, the tests — is
touched when that happens. Retreating to a hole later would be deleting a capability that
other things will already be using. **The permanent part is point 3**: views belong to
`render` and to every program, not to the editor.
Reversibility: **moderate.**

## Consequences

- **The picture works in a headset.** A view's image on a panel standing in the world is a
  floating monitor, which a hole can never be. ADR-0141 point 7 still stands — a headset
  user's own scene view is theirs to write — but the flat editor's views are no longer the
  thing that fails to carry over.
- **Drawing the world twice costs twice.** Two scene views are two passes over the scene;
  Blender pays the same and so will we. Views that are not visible are not drawn.
- **The first scene-view card is not the first card of the arc.** It waits for the view API
  (D-257), and `render` gets a card before `editor` does.
- **The first editor rendering card now exists in prospect**, which is the condition D-078
  (previewing a tier it is not running on) and D-087 (whether the editor is bound by the
  performance philosophy) were parked on. Both are deferred, not blocking, and come live
  when that card is written.
- **The element path grows a kind.** Today a panel's contents are solids and glyphs; showing
  a picture is an element that samples a texture over its bounds. Whether that is a third
  element kind or a mesh drawn with a shading record is part of D-257.

## Rejected options and why

**A — a hole.** Several holes are possible, so it is not rejected on count. It is rejected
because every hole is the same picture cut into rectangles — one resolution, one way of
drawing, flat against the camera — which cannot hold baking information beside a render
result, cannot stand on a panel in the world, and cannot have anything drawn over it
without special-casing. It would have been one card now and a rewrite at the second view.

## Questions this opens

- **D-257** — the shape of `render`'s API for more than one view per frame, and how a view's
  image reaches a panel. Hot, and the next decision.
- **D-258** — what a *way of drawing* a view is — the final picture, a debug channel, baking
  information — and whether it is a pipeline, a shader switch or a caller's choice of what
  to draw. Parked on the first view drawn other than as the final picture.
