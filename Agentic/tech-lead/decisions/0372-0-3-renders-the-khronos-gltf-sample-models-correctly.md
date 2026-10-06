# 0372 — 0.3 renders the Khronos glTF sample models correctly
date: 2026-10-06
by: tech-lead

## Decision
0.3 is no longer the Amazon Bistro. It is the Khronos glTF Sample Assets
(github.com/KhronosGroup/glTF-Sample-Assets, `Models/Models.md`) drawn correctly in the editor, judged
side by side against the same model in Khronos's own glTF Sample Viewer. The engine leans on what Vulkan and the GPU do in
hardware wherever the sample models allow it. Which models, which extensions, what "hardware" covers and
in what order are 0.3's roadmap, decision 0373. Work order 066 (the Bistro loads and draws) is withdrawn unbuilt.

## Reasoning
The sponsor changed their mind on 2026-10-06. The sample models each test one thing and come with a
reference picture, so "correct" can be checked model by model, where the Bistro could only be judged
as a whole. 
- Keep the Bistro: one big scene, a licence that keeps it out of git, and correctness judged by eye.
- Both, samples first then the Bistro: kept open as a later release.

## Replaces
0368.
