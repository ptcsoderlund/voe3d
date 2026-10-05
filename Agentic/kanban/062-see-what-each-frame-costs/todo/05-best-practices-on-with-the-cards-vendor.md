# 05 — Best Practices on in a debug build, with the chosen card's vendor checks
folder: render
after: 04
decisions: 0168, 0358, 0367

## Change
- `render/src/instance.c`: in a debug build with the layer, turn Best Practices on per 0367 point
  4: `VK_EXT_layer_settings` when the layer offers it (asked of the layer by name), with
  `VkLayerSettingsCreateInfoEXT` setting `validate_best_practices` and the four vendor sets; else
  `VkValidationFeaturesEXT` with the Best Practices bit. Record which on the device. The messenger
  gets the device as its user data; `debug_message` hands each message to the classifier below and
  prints it with its `pMessageIdName` and whether it was allowed. A debug build without the layer
  records that the checks are missing. Header: Best Practices and vendor checks, the missing layer,
  and that a new message now counts (0358).
- `render/src/card.c`: keep the chosen card's `vendorID` on the device.
- `render/src/device_internal.h`: the device's checks state: on, missing, vendor checks or not,
  the card's vendor id, the count of new messages.
- New `render/src/best_practices.c`, its group in `render/src/device_calls.h`:
  - the allowlist table (0367 point 5), empty here; card 06 fills it;
  - `bool voe_render_best_practices_allowed(const char *id_name)`;
  - `bool voe_render_best_practices_other_vendor(uint32_t vendor_id, const char *id_name)` — the
    id carries a vendor tag (NVIDIA 0x10DE, AMD 0x1002, Arm 0x13B5, IMG 0x1010) not the card's;
  - the classifier: drop another vendor's message; count an error, or a warning not allowed, as
    new;
  - `void voe_render_best_practices_announce(const voe_render_device *device)`: 0367 point 7's
    line through base's report, `render` as the source.
- `render/src/device.c`: announce right after the card is chosen, on both the window and headless
  paths. Its order comment names the step.
- `render/src/src.md`: an entry for `best_practices.c`; fix `instance.c`'s and `card.c`'s.
- `render/tests/card.c`: `voe_render_best_practices_other_vendor` on made-up ids: an NVIDIA id on
  an AMD card true, on an NVIDIA card false, an untagged id false. Its `tests.md` entry.

## Done when
- `ctest --test-dir build/debug -R '^render/card$'` passes and the folder's tests pass.
- `build/debug/render/voe_test_render_passes 2>&1 | grep -i 'best practices'` prints the line.
