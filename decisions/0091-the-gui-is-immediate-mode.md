# 0091. The GUI is immediate mode, with retained layout and one mesh per panel

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-130

## Context

The principal asked directly, having set the look in ADR-0088:

> What do you propose we do for the gui layer? I dont want XAML or WPF like which
> windows 10 was built on.

Worth separating, because ADR-0088 already relies on it: **Windows 10's *look* and
its *framework* are independent.** The shell is XAML over WPF's descendants; the
flat, square, accent-driven appearance owes nothing to that and can be produced by
anything. Refusing the framework does not weaken the style target at all.

This is card 023's *"whether the layout model is retained or immediate — a real
decision with an ADR, not an implementation detail"*, and it is D-130's blocker:
ADR-0087's theme cascade says an override applies to a control **and all its
children**, which presumes a tree. Whether there is a tree is this decision.

Constraints already fixed, and they do most of the work here:

- **ADR-0084 — transient geometry.** Built during a frame, drawn, forgotten. Reset
  every frame; the engine caches nothing.
- **ADR-0085 — a mesh's geometry is replaceable** by a direct call, geometry being
  the only mutable field.
- **ADR-0081 — submission is the binding cost**: roughly 1–3 ms per thousand
  objects drawn one at a time, against 0.02–0.2 ms to sort the same number.
- **Rule 6 — no `**`.** One level of dereference, not to be worked around with a
  typedef. **Rule 11 — memory is an arena you are handed**, freed by rewinding.
- **ADR-0089 — authored in millimetres.** **ADR-0090 — the GUI takes a font.**
- **ADR-0009 — the editor is wanted**, and is what a GUI eventually serves.
- **ADR-0083 — the GUI is a module**: it produces things to draw.

## Options considered

### Option A — retained tree, built in code

Widgets are objects in a tree, created once and mutated. No markup — the tree is
built in C. Qt Widgets without the `.ui` files; Flutter's shape without Dart.

- **Makes easy:** proper two-pass layout, the theme cascade as a literal tree walk,
  card 023's *draw once and reuse* panel, animation, and accessibility.
- **Costs:** every widget is an allocation with a lifetime, in a language with no
  destructors and a rule against double pointers — so a tree becomes arrays of
  generational ids, which works but is machinery. Rule 11 pushes toward an arena
  per panel, which is workable.
- **Makes permanent:** the shape that grows into WPF. Data binding is the next
  thing everyone asks for once a tree exists, and it is asked for because a tree
  makes it look easy. **This is the direction the principal refused**, and refusing
  it while building its substrate is a decision that will not hold.

### Option B — immediate mode

The interface is a function called every frame. A widget is a call that draws and
returns its result in the same breath — `if (voe_ui_button(ui, "Save")) { … }`.
Hierarchy is nested `begin`/`end` calls; there are no widget objects and nothing
to bind to. Dear ImGui and egui's shape.

- **Makes easy:** everything about C. There are no lifetimes, because nothing
  outlives the frame. The theme cascade becomes push/pop on a stack rather than a
  tree walk, which is *simpler* than the retained version and answers D-130
  outright. State that must persist — which panel is open, where a scrollbar sits —
  is a small keyed table, not a tree.
- **Costs:** layout is the known weakness. A pure immediate system does not know
  how big a thing is until it has drawn it, which is why ImGui's auto-layout is
  poor and why its windows jump on the first frame.
- **Makes permanent:** no accessibility. A screen reader needs a tree to walk and
  there will not be one. See the consequences; this is the real price.

### Option C — immediate calls, retained layout

Option B's API, with the system keeping a small keyed record of what was built last
frame so layout can run two passes — measure, then arrange — and so a widget's
identity survives across frames. Clay, egui and Nuklear all live around here.

- **Makes easy:** Option B's ergonomics with layout that actually works. Sizes are
  known before anything is drawn, so a row can be *as tall as its tallest child* on
  the first frame rather than the second.
- **Costs:** the keying scheme, which is where these systems get their confusing
  bugs — two widgets sharing an id silently share state. It needs to be designed,
  not improvised.
- **Makes permanent:** nothing Option B does not, and it removes Option B's worst
  weakness.

## Decision

**Option C, and the principal chose it.**, and the deciding factor is that **the mechanism it needs was built
last week for something else.**

ADR-0084 gave the engine geometry that is built during a frame, drawn and
forgotten, with the engine caching nothing; ADR-0085 gave a way to swap what an
entity draws each frame. Those two were taken for changing text, and together they
are *exactly* an immediate-mode GUI's runtime: lay out the widgets, build one mesh,
swap it in, forget it. A retained tree would use almost none of it and would want
its own persistence machinery beside it.

Three supporting reasons:

1. **It is the structural opposite of what was refused.** Immediate mode cannot
   grow into XAML — there is no *persistent* tree to bind to. The frame's nested
   `begin`/`end` hierarchy is gone by the time anything could bind to it. Option A's
   tree can, and would.
2. **Draw count stops scaling with widget count.** A panel with two hundred widgets
   costs a handful of submissions rather than two hundred, which keeps the whole GUI
   clear of ADR-0081's 1–3 ms per thousand objects and of the deferred scale bundle.
   A retained tree of widget entities would have walked straight into it. **Not one
   draw — see the four questions below**; it is one per distinct shading record,
   which today is five to ten for a panel and could become one under D-132.
3. **C.** No destructors, no double pointers, arena memory. Option B/C need
   essentially no lifetime management; Option A needs a careful one.

## The principal's reasoning, which is better than the tech lead's

He hesitated across three exchanges and then landed:

> I would like to say godot is a good rolemodel. Maybe a little bit on the complex
> side. But a good direction. managing and synchronizing state is tough and bug
> prone. I am leaning towards immediate mode. Databinding is some modern OO stuff
> that we dont want. If people want it, build it your own.

**"Managing and synchronizing state is tough and bug prone" is the strongest
argument in this ADR and it is not one the tech lead made.** The three
recommendation reasons above are about mechanism, language and cost. This one is
about the defect class: a retained tree holds a second copy of every value the
program already has, and keeping two copies in agreement is where UI bugs live. An
immediate GUI cannot have that bug because there is only ever one copy — the
program's own. That is the reason to prefer it, and the others are supporting.

### The Godot contradiction, and why it is not one

**Godot's UI is a retained tree** — Control nodes are the most retained thing there
is — so naming it as a role model while choosing immediate mode looks
contradictory. It resolves once Godot is split into the parts being pointed at:

| Part of Godot | Bearing on this decision |
|---|---|
| Widget inventory and naming | **Taken as direction.** Independent of how a widget is called. |
| Containers and anchors for layout | **Taken as direction.** It is what good immediate layout does anyway. |
| Theme system | **Not taken** — the principal already rejected it as *too much hassle* (ADR-0087). |
| Node tree authored in the editor | **The only retained part, and it is an authoring question, not an API one.** |

That last row is the whole resolution. **An editor can save a layout as data and an
immediate runtime can walk that data every frame.** The designer's artefact is a
tree in a file — ADR-0073's sectioned line format is already the shape for it — and
the developer's code is immediate calls. They coexist, and the tech lead's earlier
claim that a visual UI designer would flip this recommendation was **wrong**: it
would add an authored format, not a retained runtime.

**"Maybe a little bit on the complex side" is direction on scope**, and it is
recorded as such: Godot's Control set is large and deep. The target is that
inventory, shallower. That belongs to whoever cuts the GUI cards.

### Data binding

> Databinding is some modern OO stuff that we dont want. If people want it, build
> it your own.

**Adopted, and it does not conflict with the anti-fork rule.** ADR-0083 says a
switch exists so nobody has to modify the engine — and binding over an immediate
GUI is a layer somebody writes *in their own code*, reading their own model and
calling the public API. It needs no engine source and is therefore genuinely
opt-in rather than a fork anyone is forced into. This is the case ADR-0083's test
is happy with.

## The price, stated plainly

**Accessibility is out of reach and it is not recoverable later.** A screen reader
walks a tree of elements with roles and labels; an immediate-mode GUI has no tree
to walk, only a list of rectangles it drew this frame. Systems that have tried to
retrofit this — ImGui among them — have not got there.

That is very likely acceptable for a game engine's editor and for the games built
on it, and it is unacceptable for some products someone might one day build with
this. **It should be accepted knowingly rather than discovered.** If it is not
acceptable, that is the strongest argument for Option A and it should be made now,
because it does not get cheaper.

**Raised twice, and answered on the third: accepted deliberately.** The principal:
*"Yes, no screen readers. We are making games."* So this is a weighed choice and
not a default, and the row is closed rather than left as the thing that would
reopen the ADR.

**What that scopes, and it is worth being exact.** The decision is taken on the
premise *we are making games*. An engine is a general tool and somebody may one day
build something on it where this is not acceptable — an editor shipped to a
regulated customer, a kiosk, a training application. That is not a reason to
revisit now, and it is the shape of the thing that would be.

Two smaller prices:

- **Animation and transitions are manual.** Windows 10's are subtle and few, so the
  target is forgiving, but every animation is explicit state a program keeps.
- **A widget's identity is a key, not a pointer.** Get the key wrong and two
  widgets share state, which reads as a bug in the widget rather than in the call
  site. This is the one part of Option C that must be designed carefully.

## What this would pin

1. **The API is immediate**: a widget call draws and returns its result. Hierarchy
   is nested `begin`/`end`; there are no widget objects, no addressable tree in the
   public surface, and nothing to bind to.
2. **Layout is two-pass over a keyed record** kept between frames — measure, then
   arrange. Sizes are correct on the first frame.
3. **A panel builds its geometry each frame** through the transient pool, swapped
   in by ADR-0085's setter — **one mesh per distinct shading record, not one per
   widget.** That is five to ten draws for a Windows-10-shaped panel today, because
   colour and the distance-field flag both live in the shading record rather than on
   the vertex. One draw for the whole GUI is reachable via D-132's palette and is
   deliberately not taken here.
4. **The theme cascade is a stack.** Push a theme, build the subtree, pop. That is
   ADR-0087's *applies to a control and all its children*, and D-130 closes with it.
5. **Persistent widget state is a small keyed table** owned by the GUI, not by the
   ECS. Scroll positions and open/closed flags are not components.

## Four questions from the principal, and one correction

Asked before deciding, and the third is a correction to this ADR as first written.

### Does the whole GUI become one draw call?

**Not today, and the reason is precise.** A draw is one geometry range plus one
object record, and that record names one shading record. **Colour lives in the
shading record, not on the vertex** — `voe_render_vertex` is
`{position, normal, uv}` — so one draw is one colour. A second, independent
blocker: `base_colour_distance_field` is also per shading record, so **glyphs and
solid fills cannot share a draw** whatever is done about colour.

So a Windows 10-ish panel is five to ten draws — background, border, button faces
per state, text. Against ADR-0081's thousand-draw budget that is noise, but it is
not one.

**One draw is reachable and the route is already open as D-132**, which asks
whether a theme reaches the shader as computed colours or as a palette index. The
palette is the natural fit — a theme *is* a small derived set of colours — with a
per-vertex index and a per-vertex distance-field flag. It costs a vertex format
change that every model in the engine pays for in pool space.

**Recommendation: not yet.** Ten draws per panel is free. Take it when an editor
measures, on D-132, as a `render` decision rather than a GUI one.

### Is nothing cached — is it all rebuilt every frame?

**The GUI may cache, and ADR-0084 already permits it.** That ADR's rule is that the
*engine* caches nothing while *a caller may cache its own work, in ordinary memory,
with no help from render*. The GUI is a caller. Nothing new is needed.

Three separable costs, and they are not equal:

- **Building the geometry** — negligible, by ADR-0084's own arithmetic.
- **Running layout and widget logic** — not necessarily negligible at editor scale.
  **This is the one worth caching if anything is**, and it is what Option C's
  retained layout record already holds between frames.
- **Rendering to an offscreen target and reusing the pixels** — the largest saving
  for a complex, rarely-changing panel, and D-140's surviving argument.

**The catch is the one that killed the text cache: knowing something changed.** A UI
scale change is a *safe* invalidation because it is explicit. *Did any label's
string change* is not, and that is the one that draws yesterday's value. **The rule
worth carrying: cache against explicit signals, never against inferred ones.**

### Is there no hierarchy?

**There is hierarchy, and this ADR's first wording was misleading.** Nesting is how
it is built:

```c
voe_ui_panel_begin(ui, …);
    voe_ui_row_begin(ui);
        voe_ui_button(ui, "Save");
    voe_ui_row_end(ui);
voe_ui_panel_end(ui);
```

That is a tree. It is expressed as a **stack that exists during the frame** rather
than as objects that persist between frames. What immediate mode lacks is a
*persistent, addressable* tree — one you can hold a pointer into and mutate from
outside, which is what grows into data binding and is what the principal refused.

**Hierarchy yes; widget objects no.** The phrase "no tree" is struck.

### Can a parent scale, with children scaling too — a runtime UI scale?

**Yes, and it is an argument for this option rather than against it.** A transform
stack: push a scale, everything built inside inherits it, pop. The same mechanism
as the theme cascade.

**Runtime UI scale falls straight out of ADR-0089.** A GUI unit is 1 mm; scale at
1.5× means a unit is 1.5 mm. Push it at the root and the whole interface re-lays-out
at the new size — text, spacing and wrapping together, not a zoom of a fixed
picture.

**And re-laying-out is free, because it already happens every frame.** A retained
tree would have to invalidate and re-measure the entire tree on a scale change.
This is the clearest case where the principal's own requirement is cheaper under
Option C than under Option A.

The tension to name: that is true **until** the GUI caches. A cached mesh must be
discarded when the scale changes — which is fine, because a settings change is
exactly the explicit signal the caching rule above asks for.

## Blast radius

**Load-bearing.** This shapes every widget call anyone ever writes and the theme
cascade's meaning. Reversing it after an editor exists means rewriting the editor.

What stays cheap: the *drawing* half is nothing but transient geometry and a mesh
swap, both of which serve either model. What is expensive to change is the API
shape and the cascade, which is why this is an ADR before a card rather than a
detail inside one.

Reversibility: **load-bearing.**

## Consequences

- **D-130 closes.** The cascade is a stack, and it does not need the widget-tree
  decision it was waiting on.
- **Card 023's offscreen panel needs re-justifying.** It was argued for as *a widget
  tree drawn once and re-used across frames*. Under this model a panel is one draw
  and rebuilding it costs a fraction of a millisecond, so the saving mostly
  evaporates. What survives is a different case — a distant panel rendered at a
  fixed resolution so it stays legible, which is an entirely different argument.
  New row; the card should not build it on the old reasoning.
- **Hit testing is straightforward and stays 3D.** A ray from the pointer, per card
  023, tested against the rectangles laid out this frame. Immediate mode's usual
  one-frame lag is avoidable because the layout pass already knows the boxes.
- **The GUI holds state the ECS does not see**, which is a departure worth naming
  in an engine where everything else is a component.
- **The consequence I do not like** is the accessibility one, above.

## Questions this opens

- **D-139 — the widget identity scheme.** Call-site derived, explicit strings, or
  hashed paths. The one part of this that reliably goes wrong. Trigger: the first
  widget card.
- **D-140 — whether card 023's offscreen panel survives**, and on what argument.
  Trigger: whoever cuts the GUI into cards.
- **D-141 — where persistent widget state lives** and how it is reset. A keyed
  table owned by the GUI, but the arena rules make its lifetime a real question.
  Trigger: the first widget with state.
