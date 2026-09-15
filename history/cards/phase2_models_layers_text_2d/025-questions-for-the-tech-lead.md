# 025 — questions for the tech lead

Written by the coder on 2026-09-06 while implementing
`kanban/todo/025-text-sharp-at-any-size.md`. **None of these blocked the card.**
Every one was resolved by taking the narrowest reading and continuing, and each
entry says which reading was taken, so the answer is either "yes, that" or a
change with a known shape.

Ordered by how much a different answer would cost to act on later.

---

## 1. `voe_text_raster_fill` is now dead, and I kept it

**The situation.** The sheet is a distance field, so `text/src/font.c` no longer
calls `voe_text_raster_fill`. Nothing else in the engine ever did. The only
caller left is `text/tests/raster.c`.

**Why that is a question and not a tidy-up.** Rule 10 says a function is written
when something calls it, and the same argument run backwards says a function
nothing calls is not carried. But the card says *"Nothing else in `text`
changes"*, and deleting a public-within-the-folder function is a change it did
not ask for.

**The reading taken: keep it.** Deleting is the wider and less reversible of the
two, and the coverage rasteriser is not merely lying around — the winding rule it
is built on is what the field's sign correction asks, and the fill's own tests
are the only place non-zero winding is pinned down at all. So `raster.c` is now
two halves that share a flattener rather than one half plus a corpse.

**What the answer changes.** If it should go, it is `voe_text_raster_fill`,
`add_span`, and the five coverage cases in `text/tests/raster.c` — perhaps sixty
lines, and the four new field cases stay. It is a small, clean deletion at any
later date and nothing gets harder by waiting.

---

## 2. Where the field's sign comes from — the card's sentence, read carefully

**What the card says.** *"The existing flattener and the existing non-zero
winding rule give the magnitude and the sign respectively."*

**Why that cannot be taken literally.** If all three channels take their sign
from one global inside/outside test, the three differ only in magnitude, the
median is then just a magnitude with a shared sign, and **the corner
reconstruction is gone** — that arrangement rounds every corner exactly as a
one-channel field does. The whole reason three channels beat one is that near a
corner they are allowed to *disagree about which side they are on*.

**The reading taken.** Each channel's sign comes from the orientation of the edge
it measured — which side of that edge's own line the texel is on. That is not a
different fact from the winding rule: `cross()` in `raster.c` already reduces an
edge's orientation to `+1` or `-1` and that is the whole of non-zero winding, so
both halves of the file are reading the same property of the same contour. I take
the card's sentence to mean *contour direction is what makes inside inside*, which
is true of both, rather than *ask the scanline fill per texel*.

**Where the winding rule is genuinely used.** Once per texel, as a repair and not
as the mechanism: if the median disagrees with the fill by more than half a
texel, the three channels are replaced by the plain signed distance. Contours
that overlap can otherwise leave three one-sided fields agreeing on a side the
shape is not on, which is a hole or a blob in one letter. The half-texel deadband
is there so that texels sitting on the outline — the ones that matter — are never
touched.

**If this reading is wrong the card is not implementable as written**, so it is
worth a sentence in the ADR either way.

---

## 3. The error correction is a sign check and nothing more

msdfgen ships a considerably larger error-correction pass that also hunts for
texels where the *interpolated* median between neighbours will produce an
artifact even though every texel is individually right. I implemented only the
per-texel sign check described above.

**The reading taken: enough for now.** Nothing in the shipped range showed an
artifact, and the fuller pass is a body of work in its own right with its own
threshold to justify. **If it is ever wanted it is a later card**, and the note
belongs somewhere before somebody rediscovers the whole subject from the
symptom.

---

## 4. `3d` gained a field the card's scope headings do not name

The card scopes `render`, `text`, and "the shader and the material". The flag
lives in `voe_render_shading_values`, but nothing outside `render` can write a
shading record directly — `dev` builds a `voe_3d_material` and
`voe_3d_material_upload` copies it across. So `voe_3d_material` needed
`base_colour_distance_field` beside `unlit`, or **nothing in the engine could
ever turn the flag on** and the card would deliver a shader branch no caller can
reach.

I read "the material" in the card's own heading as covering exactly this, and
made the change: one field, one line in the upload, and the two `.md` entries.
It is the same shape as `unlit`, which arrived the same way for the same reason.
Flagged because it is a folder the card does not list, not because I think it was
avoidable.

The same applies more mildly to `dev`, `3d/src/import.c` and the two `render`
tests, which all had to pass the new sampling argument — but the card names that
one explicitly ("Every existing call site passes `SMOOTH`").

---

## 5. One correction to the card, for whoever writes the next one

> *"The descriptor write at `render/src/texture.c` already reads a **per-slot**
> `sampler` field — the shape for more than one sampler is already there."*

It did not. `struct voe_render_texture_slot` had no `sampler` member; the
descriptor write read `device->sampler`, one handle for the whole device.

**This cost nothing** — the shape the card describes is the right one and is what
now exists: the device holds one sampler per mode, a slot remembers its mode, and
the descriptor write indexes with it. One loop over the table, exactly as before.
Noted only because the card presented it as a fact already true of the code, and
the next card that leans on a sentence like that should be able to.

---

## 6. Not a question — an observation the open question wants

**A stem thinner than about one and a half texels is a stem the sheet cannot
hold**, and the mipmap chain that used to blur that into something inoffensive is
gone. At thirty-two texels to the em, Oxanium Regular's stems are about two and a
half texels, so ordinary text is comfortable; a sign seen from across the scene
at a steep angle is not, and its strokes now break rather than blur. This is
better than it was in that it is sharp where it is present, and worse in that
where it fails it fails visibly.

The card already refuses to solve this and names analytic curve evaluation as the
right second move. Nothing here disagrees — this is only the evidence, and the
screenshots are in the card's own report.

---

# Answers — tech lead, 2026-09-06

Read in the order you wrote them. Two of the six are corrections to the card and
both are mine; none of the six changes what you built. The durable record is in
the planning root — ADR-0076 is amended, and rows D-104, D-105, D-106, D-111 and
D-112 in the decision register carry the rest — so nothing here is the only copy.

## 1. Keep `voe_text_raster_fill` — you read it right, and it now has an expiry

**Keep it.** Not as an exception to rule 10 but because the rule's own test is
about to be answered by a card already on the board: **D-106** asks whether card
023's offscreen panel draws its text through the distance-field path or its own,
and a panel rendered once at a known size is the one place in this engine where a
coverage bitmap is still the correct answer. Deleting the fill today decides that
row by removing the option, which is not a tidy-up.

So the row now says so out loud: **if 023 takes the distance-field path, the fill
and its five coverage cases go with the answer.** That converts what you were
uncomfortable about — dead code kept on a feeling — into dead code with a named
date. Your reasoning for keeping it was the same reasoning; you just did not have
the row in front of you, which is the card's fault and not yours.

## 2. Your reading is correct and the card's sentence was wrong

**Build what you built.** The sign is per edge — which side of that edge's own
line the texel falls on, from the edge's direction along the contour — and the
three channels disagreeing near a corner is the mechanism rather than an error in
it. Your objection is exactly right: one shared sign per texel collapses the
median into a magnitude and rounds every corner, which is Option B, the option
ADR-0076 rejected. A card cannot ask for Option C and specify Option B.

Your repair is also right and is now the ADR's: the winding rule once per texel,
replacing the three channels with the plain signed distance where the median's
sign disagrees by more than half a texel, with the deadband there so texels on
the outline are never touched.

**ADR-0076 is amended, not superseded** — no part of the decision moved, and the
faulty sentence was the card's, not the ADR's. It now carries a section, *Where
the sign comes from*, written from your description. You asked for a sentence in
the ADR either way; you have three paragraphs, because the next person to read it
would have made the same wrong turn.

## 3. Enough for now, and it is D-112

**Stop where you stopped.** The reduced pass is a decision and not an omission,
so it is recorded as a register row with its trigger — the first artifact that
survives the sign repair — and named in ADR-0076's consequences. Your reason is
the one on the record: the fuller pass carries its own threshold to justify, and
a threshold nothing forced would be invented rather than measured.

## 4. Correct, and the card's scope headings were wrong rather than your change

**Right call, no permission needed retrospectively.** Nothing outside `render`
can write a shading record, so without the `3d` field the card would have
delivered a shader branch with no reachable caller — which rule 10 refuses from
the other direction. It adds no module edge: `3d` already depends on `render`,
and ADR-0060 already licenses `render`'s surface growing on demand. It is the
same arrival as `unlit` under ADR-0071, as you say.

The card's scope list was incomplete; "the material" did cover it. Recorded as
scope, not as a decision — no ADR, because nothing architectural moved.

## 5. Accepted, and it is the second one in two days

**The card asserted something about the code that was not true of it.** The
per-slot `sampler` field did not exist; the descriptor write read one handle for
the whole device. You are right that this is worth flagging even though it cost
nothing, and right about why: the next card that leans on a sentence like that
should be able to.

It is the second card written under the standing card-writing grant to be caught
short by its coder in one day — card 024 named a trap and left no route through
it, this one described a shape it had not checked. Two is a pattern, so it is
**D-111** rather than two anecdotes: whether a card may state a fact about code
it has not verified, and what changes if not. You are not being asked to work
around it; it is on me.

## 6. This is the most useful thing in the file

**A stem thinner than about one and a half texels, against Oxanium's two and a
half at thirty-two texels to the em** — that is a number where **D-104** had a
worry, and the row now carries it, with your point that the failure changed
character rather than arriving: it breaks visibly instead of blurring quietly.
The row stays open and the card is right to refuse it. Analytic curve evaluation
is named there as the second move.

---

**Nothing here asks you to reopen the card.** It is finished work in `review/`
and the principal's to accept. If any of the above turns into work it arrives as
a new card, per ADR-0068.
