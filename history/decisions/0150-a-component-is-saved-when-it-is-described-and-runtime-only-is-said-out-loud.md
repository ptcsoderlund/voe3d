# 0150. A component is saved when it is described, and runtime-only is said out loud

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** ADR-0132 point 2's *NULL is allowed and means undescribed* only. The rest of
  ADR-0132 stands
- **Superseded by:** —

## Context

Asked by the principal during the grammar session (ADR-0149):

> *"Does that mean all components needs to be serializable (authored?)?"*

**No, and the switches already exist.** Whether an **entity** is saved is its identity
(ADR-0125: presence means authored, and a bullet has none). Whether a **component** is saved
has an obvious candidate: whether it has a description (ADR-0122, ADR-0132). Some components
are not authored data at all — `3d`'s mesh holds a GPU geometry id, a panel holds this frame's
element range, a material holds texture and shading ids — and none of those means anything in a
file.

**The flaw is in what NULL means today.** ADR-0132 point 2 lets a component register with a NULL
description and reads it as *a component nothing has described yet* — a temporary gap. If NULL
also means *never saved*, then forgetting to describe a component is a quiet data loss on every
save, found by whoever reopens the scene. Five of the seven components registered today pass
NULL: `3d`'s mesh, material and panel, `scene`'s camera and light.

## Options considered

### Option A — described means saved, and NULL keeps meaning undescribed
No change to registration. A forgotten description is indistinguishable from a deliberate
runtime component, and the failure is silent.

### Option B — described means saved, and runtime-only is a declared value
Registration takes either a description or an explicit runtime-only marker. NULL is a mistake,
caught at build time where it is written literally and at registration where it is not.

## Decision

**Option B**, the tech lead's recommendation, accepted by the principal.

1. **A component on an authored entity is saved if and only if its type has a description.**
   ADR-0149 says how; this says which.
2. **A type is registered with a description or with `voe_ecs_runtime_only`**, a marker `ecs`
   provides. **A NULL description is refused**: `voe_ecs_component_register` declares the
   parameter non-null so a literal NULL is a compiler error under this repository's `-Werror`,
   and asserts on a NULL that arrives any other way.
3. **The world answers which it is.** The description a type hands back stays NULL for a
   runtime-only type, so every existing reader keeps working, and a runtime-only query says so
   explicitly for the one caller that must tell the two apart — the writer.
4. **A runtime-only type is still listed and still shown by name** in the inspector, unexpanded,
   exactly as an undescribed one is today (ADR-0132 point 6 is unchanged).
5. **The seven types registered today are assigned now**, because which is which is content and
   not a coder's judgement:

   | Type | Is | Because |
   |---|---|---|
   | `voe_scene_identity`, `voe_scene_transform` | described already | — |
   | `voe_scene_camera` | **described** | eye, yaw, pitch, field of view and planes are authored placement, all in the vocabulary |
   | `voe_scene_light` | **described** | direction, colour and intensity are authored, all in the vocabulary |
   | `voe_3d_mesh` | **runtime-only** | a GPU geometry id; its authored half is an asset reference (D-259) |
   | `voe_3d_material` | **runtime-only** | texture and shading ids; its authored half arrives with the same asset reference |
   | `voe_3d_panel` | **runtime-only** | this frame's element range |

## Blast radius

**Cheap.** One parameter's contract and seven registrations. Reversing it is allowing NULL back.
The permanent part is the rule in point 1, which ADR-0149's writer is built on.
Reversibility: **cheap.**

## Consequences

- **A forgotten description is a build error, not a silent loss.** The cost is one word at every
  runtime registration, which is also documentation.
- **Describing camera and light makes them appear expanded in the inspector.** They are shown
  and not edited until their systems accept a replace intent (ADR-0134); that is a later card
  and nothing here requires it.
- **A saved scene loses its meshes and materials** until D-259 gives them an authored half. The
  save arc has to reach that question before a reopened scene looks like the one that was saved.
- **Editor-only components (ADR-0126) are described and therefore saved**, and the cook strips
  them. Unchanged.

## Rejected options and why

**A — NULL keeps meaning undescribed.** It makes the most common mistake a new component can
have — no description — invisible until a scene is reopened, possibly by somebody else, possibly
after the only copy was saved over.

## Questions this opens

None. D-259, the asset reference that gives mesh and material an authored half, is opened by
ADR-0149.


## Amendment · 2026-09-13 · a build without descriptions says so with a second marker

**Found by the coder of card 069**, before any code was changed: *"With descriptions off,
`transform_description()` and `identity_description()` return NULL (their `#else` branch), and
step 6c of check.cmake runs the scene tests in exactly that build. Once registration asserts on
NULL, those tests abort and check goes red. […] What should a descriptions-off build pass?"*

**Point 2 was written for a build with descriptions on and did not say what the other build
does.** A game's own tree leaves them off (ADR-0128 point 3), `check.cmake` step 6c keeps that path
compiling (ADR-0145 point 5), and there the accessor a described type registers with does not exist.
Nothing in such a build saves or inspects, so the silent loss point 2 guards against cannot happen
there; the question is only what to pass without weakening the guard where it matters.

**Decided by the principal, on the tech lead's recommendation:**

1. **`ecs` provides a second marker, `voe_ecs_description_compiled_out`**, beside
   `voe_ecs_runtime_only`, compared by address the same way. It means *this type is described, and
   this build compiled descriptions out*.
2. **A described type's `#else` branch returns that marker** instead of NULL. NULL stays refused in
   every build, by the non-null declaration and by the assert.
3. **The world answers it honestly**: its description is NULL, and runtime-only is **false** — the
   type is authored data whose table is not in this binary.

**Rejected.** *Pass runtime-only in the off build*: no new name, but a transform would say it is
runtime-only, and the word point 2 made into documentation would stop meaning what it says. *Let NULL
through when descriptions are off*: the switch is read per translation unit (`base/describe.h`), so
`ecs` and the registering file can disagree about it, and NULL would regain a meaning.

**Blast radius: cheap.** One extern and one comparison in `ecs`; one line in each described type's
`#else` branch. Card 069 carries it.

