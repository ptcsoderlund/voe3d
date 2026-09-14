
## The two shader compilers are two years apart — measured, 2026-09-10

**The principal's Windows machine:**

```
slangc -v
2024.17-1-g839bc9aa
```

**This Linux workstation: `slangc` 2026.13.1-1-g84792eb15**, recorded in card 009's
notes (`kanban/complete/phase1_pre_study/009-triangle.md:128`) and **never recorded
anywhere since** — which is the hole ADR-0112 closes and, as it turns out, the only
reason the comparison is possible at all today.

**So the two platforms have never been running the same shader**, and the gap is not
a point release. It is roughly two years of a compiler that is under active
development, on the one tool in this project whose output goes to the GPU
untranslated.

**Every shader in this engine has only ever been verified under 2026.13.1.** The
Windows binary has been built by 2024.17 since card 009 and nothing has ever
looked at what it emits.

### What that does to the hypothesis

The doubling needs 2024.17 to recognise `SV_StartInstanceLocation` — it must, or the
Windows build would fail to compile and it does not — **while still lowering
`SV_InstanceID` to `gl_InstanceIndex`**, which already carries `firstInstance`. Then
`first + instance` is `firstInstance + (firstInstance + i)`. That is a coherent
intermediate state for a compiler that later moved `SV_InstanceID` to HLSL's meaning,
and this file's own header states that later meaning as though it were a fact about
Slang rather than about one version of it:

> Slang gives SV_InstanceID HLSL's meaning: the instance's number within this draw,
> counting from nought no matter what firstInstance was.

**True on 2026.13.1. Unverified on 2024.17, which is the compiler that built the
binary with the fault in it.**

**Two alternatives are ruled out by the failing checks, not by argument.** If 2024.17
treated the semantic as an ordinary varying input — reading zero, since no vertex
buffer is bound — the index would come out as either `first + i` (correct, nothing
would be wrong) or `i` (records 0..count-1). Under `i`, `a_mesh_after_an_element_draw_is_still_right`
draws record 0 twice, the bottom-right quadrant is green, and that check passes. **It
failed.** So neither is what is happening.

### The experiment this makes available, and it needs no code

**Install a current `slangc` on the Windows machine and rebuild.** If the ticks, the
plate, the bar and the interface come back, the compiler is the cause and no
disassembly is needed from anybody.

- **Keep the old installation** rather than replacing it, so the fault stays
  reproducible. It is the only machine that has it.
- **Two years of Slang may not compile these shaders unchanged.** If the build fails,
  that is a finding and not a setback — report what it says. Nothing about the engine
  is committed to by trying.
- **It is a diagnostic and not the fix.** If a compiler update makes the widgets
  appear, the engine still relies on one version's reading of one semantic and still
  has to stop doing that.

`probe_which_record` is cheaper still and is already in the tree, so it answers first
if the check is being run anyway. The two agree or the hypothesis is wrong.

### What the floor should be, and it is not the older of the two

ADR-0112 said the floor comes from the two machines in hand, **the older of the two
where both are known good**. 2024.17 is not known good — it is the suspect. **The
floor is 2026.13.1**, the only version any shader in this engine has ever been
verified under. Card 045 carries the number.
