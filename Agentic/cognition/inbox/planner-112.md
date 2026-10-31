## Assumed
- Fog thickness's top is a named constant, `VOE_SCENE_FOG_VOLUME_THICKNESS_MAX` in `scene/fog_volume_component.h`, shared by the range and the refusal.

## Learned
- Fog thickness's bound is enforced only by the scene system's `fog_volume_valid()`; the Inspector's slider takes its ends from the description's range, so no editor change is needed to widen it.
- At 1.6/m the fog march's 32 steps put the front surface, soft edges and godrays at one or two samples per pixel with a fixed per-pixel jitter: thick fog may show a still grain, not banding or flicker; unrendered, unverified.
