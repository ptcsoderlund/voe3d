# 0142. The editor's panel layout is a tree of data, and a root is a camera and a panel set

- **Status:** Accepted
- **Date:** 2026-09-12
- **Deciders:** Tech Lead, on the principal's explicit delegation
- **Supersedes:** — (builds on ADR-0141; splits card 058)
- **Superseded by:** in part, by ADR-0153 — point 6's *scrolling and clipping*

## Context

The principal asked for the Godot arrangement:

> I think we need to plan for editor layout. We want 3d scene editor. Something that is like
> the tree view in godot. Inspector and so on. We want to be able to rearrange them as panels
> however the dev like. Also detach them into another window, which also can be rearranged
> with panels.

ADR-0141 settled what the interface is made of — world geometry at millimetre scale, no
snapping, a replaceable camera, and a pointer handed in rather than computed. What it left open
is the question that gates card 058: **does the editor's panel layout come from data the
interface walks each frame, or from C call order?** Card 058 as written is call order — a panel
down the left, a heading on the right, in sequence in `interface.c`.

The tech lead asked twice and the principal declined to choose: *"I leave that decision up to
you. You are better competent then me."* **This ADR is therefore taken by the tech lead under
delegation, and is recorded as such** so that a later reader knows whose decision it was and who
owns reversing it.

Constraints already fixed:

- **`ui` is already nested rows and columns** of boxes in millimetres, with fill, fit and fixed
  sizing, padding, gaps, anchored children and the measured size read back. A dock layout *is*
  nested rows and columns with draggable boundaries, so the layout engine is built and tested.
- **Nothing in `ui` survives a frame, by design** (ADR-0093, ADR-0095). Anything that persists
  between frames — a split the developer has dragged — therefore lives in the caller.
- **ADR-0091** — the GUI is immediate-mode and re-laying-out every frame is free, so walking a
  tree every frame costs nothing worth naming.
- **ADR-0141 point 5** — a panel's transform is the editor's to choose, and the editor's panels
  are locked to its camera by default.
- **D-148 is parked** on whether the editor authors UI layouts *as data in the sectioned text
  format*. That is the file question. This ADR is the runtime question and does not answer it.
- Cards 058 and 059 are unclaimed. Card 057, the number box, is claimed by a coder and is
  unaffected by every option here.

## Options considered

### Option A — call order in C, revisit at docking
`interface.c` opens a column, a panel, the list, then the inspector, in sequence. Smallest
possible 058. The editor's interface file and every card added to it between now and docking is
rewritten once, and the size of that rewrite grows with every panel.

### Option B — a tree of data in `editor/`, walked each frame
The editor owns nodes: a split with an axis and a fraction, or a leaf naming a panel. The
interface walks the tree and emits `ui` rows and columns from it. 058 grows by about a third and
one file. Everything later — a splitter that writes the fraction back, tab bars, dragging a
panel elsewhere, a second root — is additive, because the geometry already comes from data that
outlives the frame.

### Option C — the tree in `ui`, so every program gets docking
Same mechanism, one folder down, available to games. `ui` gains state that survives a frame,
which is the property it was explicitly built without, and we would be generalising for exactly
one caller.

## Decision

**Option B**, with limits that are the real content of this decision. The deciding factor is
that the principal has named the destination on the record, so a layout that comes from data is
not generality without a caller — the caller is the docking he asked for — while Option A's bill
is paid in a rewrite that grows every card.

1. **The tree lives in `editor/`.** Not `ui`. Revisited when a second program wants docking,
   and not before.
2. **A node is a split — an axis and a fraction — or a leaf naming one panel.** The walk emits
   `ui` rows and columns; a leaf calls the function that draws that panel's contents.
   **Panel contents stay ordinary C.** The tree owns geometry and never content, which is what
   keeps it small: all the code is in the contents, and none of it is affected by this decision.
3. **A leaf names one panel, not a list of panels with an active tab.** Tabs are cheap to
   retrofit — a leaf grows a field, the walk grows a tab bar, the tree's shape does not change —
   so they are not built. **This is the test applied throughout this ADR: take what is expensive
   to retrofit, decline what is cheap.** It is also the test Option A fails.
4. **The roots are an array, holding one entry.** A root is a camera, a pointer and a tree. The
   main window is root zero; a detached window is a further root, and under ADR-0141 so is a
   headset's eye. This costs one loop today and names the thing a detached window will be.
5. **Point 4 does not de-risk the second window, and must not be read as doing so.** `platform`
   opens a window, `render` opens a device onto a window, and `app` pairs the two. A second OS
   window is real work in those three folders, and the editor's root array cannot make it
   cheaper. All point 4 buys is that the editor will not *additionally* need untangling when
   that work happens.
6. **Not built, each deliberately**: dragging a splitter, tab bars, dragging a panel into
   another leaf, closing a panel, scrolling and clipping, saving the layout, and a second root.
   The tree is built in code and **a person cannot rearrange anything** until a later card.
7. **Card 058 splits into 058a and 058b.** 058a is the window and the tree with two leaves;
   058b is the scene and the list of what is authored. The reason is scheduling, not size:
   **058a needs no identity component**, so it can be claimed immediately while card 055 is
   still open, and it proves the tree's shape with nothing built on top of it yet.

## Blast radius

The tree is the editor's own data in the editor's own folder. Changing its shape touches the
node type and the walk — two functions in one file — and no panel's contents. Declining tabs
(point 3) costs a field and a widget when they arrive. Declining the second root would have
cost a loop.

What this avoids is the one expensive thing in the area: retrofitting data-driven geometry
underneath an interface written as call order, whose cost is proportional to how many panels
exist by then.

**Reversibility: cheap.**

## Consequences

- **Cards 058 and 059 are rewritten before either is claimed**, and 058 becomes two cards. Card
  057 is untouched.
- **The first editor card gains a file and roughly a third more work, and you see exactly the
  same two panels you would have seen anyway.** The whole payoff is later. This is the cost and
  it is accepted knowingly.
- **A tree built in code that nobody can rearrange will read as pointless indirection** to
  someone opening `editor/` before the splitter card lands. This ADR is the answer to that
  reader, and the editor's folder page should point at it.
- **Saving the layout is not decided here.** A layout the developer rearranges and loses on
  every restart is worse than a fixed one, so the splitter card and D-148's file question arrive
  close together, and the splitter card should not land far ahead of it.
- **Decided under delegation.** If this is wrong it is the tech lead's to supersede with a new
  ADR, and not something to put back to the principal as an open question.

## Rejected options and why

**Option A** — it is cheaper exactly once, today, and its bill grows with every panel added
before docking. The principal has already said docking is wanted, which removes the usual
defence of call order (that the future caller is speculative).

**Option C** — `ui` is explicitly a folder where nothing survives a frame, and a dock tree is
persistent state. Putting it there would generalise for one caller and weaken the property that
makes `ui` testable without a window or a graphics card. It becomes correct when a second
program wants docking.

## Questions this opens

- **D-148 gains a pointer to this ADR**: the runtime shape is settled here, the file the layout
  is saved into is still open and still parked on the first editor UI card.
- **New — D-250: whether a detached editor window is a second root of one process or a second
  process.** Godot, Unity and Blender all take one process with several OS windows, which is
  where point 5's work lands. A second process would instead need the two to agree about a world
  they both edit, which is a larger question than windowing. Parked on the detached-window card.
