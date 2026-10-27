# 0481 — A blended model reads the live ground under each pixel
date: 2026-10-27
by: planner
## Decision
Feature 097, models blending into the ground, is drawn so:

1. **Per pixel, from the live textures.** The mesh fragment stage of a blending draw reads the
   landscape's heights and paint textures where the pixel is: `above` is the pixel's height over
   the ground under it in the landscape's own metres, the weight t = smoothstep(0, 1,
   1 − above / reach), 1 below the ground. Base colour lerps to the painted layers' surface by t,
   and when lit the ORM and the normal (the ground's, its layer normal texel turned as the
   landscape's own path turns it) too. Alpha, cut-out and dither stay the model's. Nothing is
   baked, so a move, a sculpt or a repaint shows the next frame, and the editor, Play and a
   shipped build draw it alike. Shadow, point-shadow and capture passes do not blend.
2. **The object record carries the ground.** `reserved1` becomes `ground` (the landscape's heights
   texture index plus one, nought none), `reserved2` the float `ground_reach` in metres, and the
   record grows from 272 to 320 bytes by `ground_rows[3]`: the top three rows of the map from
   camera-relative space to the landscape's own metres. A ground record names the landscape's
   `paint` and `layers` and its side in `terrain.w`; its `heights` stays nought. Only a painted
   landscape (`paint[0]` not nought) blends.
3. **Which ground.** A model row blends with the first landscape row the frame draws solid whose
   square holds the row's origin in that landscape's own x and z; with none it draws as ever. A
   foliage copy blends with the landscape row it stands on.
4. **The switches.** The model component gains `blend_with_ground` (false) and `blend_height`
   (metres, 0.5 in the default row and a file without it); the draw clamps the height to 0.05–10 m
   and a height not above nought blends nothing. A kind's look gains `blends` (key
   `blend<k>=1`) and `blend_height` in centimetres, 5–1000, 0 meaning 50 (key
   `blendheight<k>`), each written only off the default. The kind's Blend with ground button
   and Blend height slider sit on its row of the Inspector's Foliage section after Cast shadows:
   that is where a kind's settings are (0473 point 2, 0476 point 1), which the feature calls the
   Landscape panel. No kind is special (0475): grass blends when the sponsor turns it on.
5. **Its cost is shown.** Blending model parts are drawn solid inside one `ground blend` span
   after the other model parts, blending foliage groups inside one `foliage blend` span after the
   `foliage` span, so the frame breakdown lists both (0388). Blended (see-through or fading)
   parts blend too, uncounted, sorted with the world's blended group as ever.
## Reasoning
A bake per model would go stale on every sculpt and paint stroke and need a cook step; the
textures are already uploaded each frame for the landscape's own draw (0396 point 5, 0402 point
4), so reading them is free of state and every move is live. The paint reads are the landscape's
own (`terrain_layers.slangh`), so the foot wears exactly the ground beside it. Two reserved words
and three rows are the least the shader needs; the rows let a landscape stand anywhere. Gating
by draw keeps every other draw's cost unchanged, and the spans make the blend's cost visible
where 0388 already lists costs.
## Replaces
Nothing.
