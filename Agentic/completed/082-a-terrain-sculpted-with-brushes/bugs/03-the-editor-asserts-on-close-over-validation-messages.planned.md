# 03 — The editor asserts on close over validation messages

## Seen
On Windows, debug build. The editor starts and runs; when I close it, it stops on an assert:

```
editor: before main 0.073 s
editor: project 0.008 s
editor: window and device 1.628 s
editor: font, themes and interface 0.064 s
editor: preparing shaders 0.195 s
editor: scene and shapes 0.482 s
editor: first frame 0.006 s
editor: total 2.455 s
error: render: vulkan warning BestPractices-PushConstants (allowed): Validation Warning: [ BestPractices-PushConstants ] Object 0: handle = 0x539a8d0, type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0x1248c6a4 | vkCmdDrawIndexed(): Pipeline uses a push constant range with offset 0 and size 64, but 48 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): ... size 64, but 32 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): ... size 64, but 16 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): Object 0: handle = 0x53970d0 ... size 64, but 48 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): ... size 64, but 32 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): ... size 64, but 16 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): Object 0: handle = 0x539a8d0 ... size 64, but 48 bytes were never set with vkCmdPushConstants.
error: render: 31 new validation messages this device: fix each, or add it to the allowlist by a decision (render/src/best_practices.c)
ASSERT  C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/render/src/device.c:833
        new_messages == 0
        a validation error or a Best Practices warning not on the allowlist was raised; see the lines above
```

Every message printed is marked "(allowed)". The 31 new ones the assert counts are not among the lines above —
the output starts at `editor: before main`, so nothing was cut off. I cannot tell what they are.

## Expected
Closing the editor ends it quietly. If a message is new, it is printed with what it says, so the line above the
assert names it. Each one is then fixed, or allowed by a decision — not by the planner on its own.

## How to reproduce
1. On Windows, build the debug preset.
2. Start the editor on a project that has a landscape model in it.
3. Do nothing; close it.
4. It stops on the assert at `render/src/device.c:833`.

Whether a project without a landscape closes cleanly is not known yet; finding that out is part of the work.
