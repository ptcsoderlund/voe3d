# 0384 — The checks source the scratch environment
date: 2026-10-08
by: tech-lead

## Decision
The Khronos validation layer the debug build's Best Practices gate (0358) needs
is part of the programmer's environment, like every other installed tool
(0004): it lives in `~/voe3d-scratch`, and `~/voe3d-scratch/env.sh` puts it on
`VK_LAYER_PATH`. Each check line in `CLAUDE.md` sources `env.sh` itself when the
file exists, so a check run finds the layer whichever shell or folder started
`drive.sh`; the programmer's profile no longer matters to the checks. The build
does not fetch the layer, and where `env.sh` is missing the gate still fails
loudly. A `render/best_practices` failure saying "could not load the validation
layer" is an environment fault, not a finding: the planner does not card it, it
raises it with the programmer.

## Reasoning
The layer is installed and the test passes once `env.sh` is sourced. Only the
shell that started the run was wrong. Sourcing `env.sh` in the check lines fixes
that inside the repo at no code cost, and does not depend on how the shell
started.
Alternatives: the build fetches and builds Vulkan-ValidationLayers (a new
repo-wide dependency and a long first build, to fix a shell problem); or a
missing layer skips the test (the gate would switch off without anyone
noticing, on exactly the runs nobody watches).

## Replaces
Nothing. Applies 0004 to 0358's layer.
