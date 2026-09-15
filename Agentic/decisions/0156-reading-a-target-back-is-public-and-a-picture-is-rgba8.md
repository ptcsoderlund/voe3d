# 0156 Reading a target back is public, and a picture is RGBA8 with straight alpha

Status: accepted
Date: 2026-09-15

Spec 003 asks for a drawn frame to be written out as a PNG file. The pixels only exist on the
card, and today the one code that brings them back is `render/tests/offscreen.c`, which resolves
`vkCmdCopyImageToBuffer` by hand and says in its header that reading a target back "is not
something the engine does, so it is not part of render's public surface and must not become part
of it so that a test can see it". A program that saves a picture is the caller rule 10 was
waiting for, so the surface is added now — for that caller and not for the tests.

## Decision

**`render` gains one call that copies a target's picture into an arena**, beside
`voe_render_target_create`:

```c
typedef struct {
	uint32_t width;
	uint32_t height;
	uint8_t *pixels;   // width * height * 4 bytes
} voe_render_picture;

[[nodiscard]] bool voe_render_target_read(voe_render_device *device,
					  voe_render_target target,
					  voe_base_arena *arena,
					  voe_render_picture *out,
					  voe_base_error *error);
```

- **RGBA8, in that byte order, whatever the images are made of.** The colour image is
  `B8G8R8A8_SRGB`; swapping the two channels is `render`'s business, because `render` is the only
  folder that knows the format. RGBA8 is what `assets` already decodes a picture to, so the two
  halves of a save meet with nothing in between.
- **Row zero is the top one**, which is what PNG, Vulkan and glTF already agree on (see
  `assets/include/assets/image.h`).
- **Straight alpha, not premultiplied.** ADR-0069 makes the target hold premultiplied colour;
  PNG holds straight. The divide is `render`'s, at the one place that knows which convention the
  image is in. The clear colour is opaque, so in practice every pixel divides by one.
- **The bytes are sRGB-encoded already** — the format carries the curve — and nothing here
  applies a gamma. That is what makes the saved colours the drawn colours.
- **It waits for the card to go idle and is not a per-frame call.** It is `voe_render_target_create`'s
  kind of operation: taking a picture stalls the pipeline, which is what an out-of-frame capture
  can afford. Calling it inside an open frame asserts.
- **It reads the slot the last ended frame drew into**, so a caller gets the frame it just drew.
  Before any frame has ended, and after a resize, the picture is undefined — the rule targets
  already have.
- **`VOE_RENDER_TARGET_WINDOW` is a target here as everywhere.** On a windowed device that is the
  offscreen pair the frame is blitted to the window from, and on a headless device it is the same
  pair with nothing to blit to, which is what makes a saved picture and a shown one the same
  picture rather than two code paths that ought to agree.

**`vkCmdCopyImageToBuffer` joins the loader's table.** It now has engine code calling it.

## Rejected

- Keep it internal and let each program reach into `render/src/` — rule 1: a folder never reaches
  into a sibling's source, and a program is not a sibling to reach with.
- Hand back the device's own byte order and let the caller swap — every caller would then carry a
  format switch `render` already has the answer to, and one of them would get it wrong.
- Hand back a texture id's contents rather than a target's — a texture made from CPU pixels is
  already the caller's own data; the picture nobody else has is the one a pass drew.
- Copy without waiting, through a fence the caller polls — a capture that returns pixels later is
  a second lifetime to get wrong, for a call made once a session or once a frame at most.

## Consequences

- `render/tests/offscreen.c` and the other headless tests keep their hand-resolved copy and their
  per-slot reach into `device_internal.h`: they check claims the public call deliberately does not
  expose, such as which slot holds which picture.
- A caller that wants a picture every frame pays a stall every frame. If that ever matters, the
  answer is a second call with a fence, decided then.
