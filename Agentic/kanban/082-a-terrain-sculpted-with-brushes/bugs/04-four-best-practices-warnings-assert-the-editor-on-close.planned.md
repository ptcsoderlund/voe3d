# 04 — Four Best Practices warnings assert the editor on close

## Seen
On Windows, debug build, after bug 03's cards (35–37). The editor still stops on the assert when I close it, and
the close now names what it counts:

```
error: render: vulkan warning BestPractices-PushConstants (allowed): Validation Warning: [ BestPractices-PushConstants ] Object 0: handle = 0x545a8d0, type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0x1248c6a4 | vkCmdDrawIndexed(): Pipeline uses a push constant range with offset 0 and size 64, but 48 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): ... size 64, but 32 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): ... size 64, but 16 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): Object 0: handle = 0x54570d0 ... size 64, but 48 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): ... size 64, but 32 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): ... size 64, but 16 bytes were never set with vkCmdPushConstants.
error: render: vulkan warning BestPractices-PushConstants (allowed): Object 0: handle = 0x545a8d0 ... size 64, but 48 bytes were never set with vkCmdPushConstants.
error: render: new vulkan warning BestPractices-vkCreateCommandPool-command-buffer-reset, 1 times, first: Validation Performance Warning: [ BestPractices-vkCreateCommandPool-command-buffer-reset ] | MessageID = 0x86974c1 | vkCreateCommandPool(): pCreateInfo->flags has VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT set. Consider resetting entire pool instead.
error: render: new vulkan warning BestPractices-vkAllocateMemory-small-allocation, 10 times, first: Validation Performance Warning: [ BestPractices-vkAllocateMemory-small-allocation ] | MessageID = 0xfd92477a | vkAllocateMemory(): pAllocateInfo->allocationSize is 49152. This is a very small allocation (current threshold is 262144 bytes). You should make large allocations and sub-allocate from one large VkDeviceMemory.
error: render: new vulkan warning BestPractices-pipeline-stage-flags2-compute, 10 times, first: Validation Warning: [ BestPractices-pipeline-stage-flags2-compute ] Object 0: handle = 0x545e0d0, type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0x2172541c | vkCmdPipelineBarrier2(): pDependencyInfo using VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
error: render: new vulkan warning BestPractices-vkEndCommandBuffer-VtxIndexOutOfBounds, 10 times, first: Validation Performance Warning: [ BestPractices-vkEndCommandBuffer-VtxIndexOutOfBounds ] Object 0: handle = 0x54570d0, type = VK_OBJECT_TYPE_COMMAND_BUFFER; | MessageID = 0xc91ae640 | vkEndCommandBuffer(): Vertex buffers was bound to VkCommandBuffer 0x54570d0[] but no draws had a pipeline that used the vertex buffer.
error: render: 31 new validation messages this device: fix each, or add it to the allowlist by a decision (render/src/best_practices.c)
ASSERT  C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/render/src/device.c:722
        new_messages == 0
        a validation error or a Best Practices warning not on the allowlist was raised; see the lines above
```

Four ids, 31 messages (1 + 10 + 10 + 10):

- `BestPractices-vkCreateCommandPool-command-buffer-reset` — 1 time
- `BestPractices-vkAllocateMemory-small-allocation` — 10 times, 49152 bytes each
- `BestPractices-pipeline-stage-flags2-compute` — 10 times, a barrier with `VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT`
- `BestPractices-vkEndCommandBuffer-VtxIndexOutOfBounds` — 10 times, a vertex buffer bound that no draw uses

Which code raises each one is not known.

## Expected
Closing the editor ends it quietly. Each of the four is fixed where it is raised, or allowed by a decision — not
by the planner on its own.

## How to reproduce
1. On Windows, build the debug preset.
2. Start the editor on a project.
3. Do nothing; close it.
4. It stops on the assert at `render/src/device.c:722`.

Whether this run was on a project with a landscape, and whether a project without one closes cleanly, is not
known yet; finding that out is part of the work.
