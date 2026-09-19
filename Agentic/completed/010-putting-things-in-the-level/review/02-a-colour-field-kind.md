# 02 — A colour field kind
folder: base
decisions: 0168, 0191

## Change
`include/base/describe.h`: add `VOE_BASE_FIELD_COLOUR` to `voe_base_field_kind`, after `VOE_BASE_FIELD_FLOAT4X4`
(before `ENUM`), and `#define VOE_BASE_FIELD_SIZE_COLOUR 12`. The header gets one paragraph: COLOUR is three
floats, linear RGB, each 0..1, laid out and spelled as FLOAT3. It exists so a tool can show a swatch without
naming the component, the way QUAT is shown as angles (0191).

`tests/describe.c`: a struct with `F(<a local struct of three floats>, tint, COLOUR)` describes as kind
`VOE_BASE_FIELD_COLOUR`, size 12, count 1, rank 0.

Downstream (ADR-0113): if adding the enumerator breaks a `switch` over `voe_base_field_kind` in
`authoring/src/scene_write.c`, `authoring/src/scene_read.c` or `editor/src/inspector.c`, add `COLOUR` beside
`FLOAT3` in that switch so it is handled exactly as FLOAT3. Touch nothing else there, and name each site in your
report.

## Done when
The folder's check passes (`checks.sh` for `base`) with the new case in `base/describe`, and
`cmake --build --preset debug` builds the whole tree.
