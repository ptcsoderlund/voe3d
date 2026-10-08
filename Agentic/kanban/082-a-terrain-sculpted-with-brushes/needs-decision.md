# Needs decision — the suite's shell has no validation layer

Blocked card: `blocked/35-suite.md`.

## What fails, and why it is not code

`render/best_practices` is the only failure. Its output, run in a shell that
has not sourced `~/voe3d-scratch/env.sh`:

    warning: render: best practices checks missing: this debug build could
    not load the validation layer, so no message is checked
    FAIL render/tests/best_practices.c:158  device->checks_on
    FAIL render/tests/best_practices.c:159  !device->checks_missing

Here the test does what 0358 asks: a missing layer fails loudly. The Khronos
validation layer is installed only in the scratch sysroot
(`~/voe3d-scratch/tools/sysroot/usr/share/vulkan/explicit_layer.d`), and
`env.sh` puts it on `VK_LAYER_PATH`. The system's `explicit_layer.d` does not
have it. `~/.profile` and `~/.bashrc` source `env.sh` only when the shell
starts inside the repo. The shell that ran `drive.sh` did not, so its
`checks.sh --all` saw no layer. With `env.sh` sourced, the same build passes
the test (checked while planning: `ctest -R ^render/best_practices` passed).

The same failure was in the 30 suite run. No card can fix it, so it will
come back on every suite run started from that kind of shell.

## Question

Where should the validation layer the suite needs come from?

## Options

1. **The environment, as now (0004: tools installed).** The programmer makes
   sure the shell that starts `drive.sh` has `env.sh` sourced. That could
   mean `drive.sh` or `checks.sh` sourcing it, or the profile not depending
   on the start directory. No cards. The 082 suite is run again from that
   shell.
2. **The build fetches the layer.** CMake fetches or builds
   Vulkan-ValidationLayers, and `voe_module` sets `VK_ADD_LAYER_PATH` on the
   tests to the fetched layer. Every suite run would then be the same
   wherever it starts. Cost: a new dependency for the whole repo, and a long
   first build.
3. **Amend 0358 so a missing layer skips the test.** The test would pass
   with a warning when no layer is found. Cheapest, but the gate would turn
   off silently on exactly the runs that are not looked at.

## Recommendation

Option 1. The rule that the environment belongs to the programmer already
covers it, the layer is installed, and only the shell that starts the run is
wrong. When that is fixed, delete `blocked/35-suite.md` and run the suite
again; no cards are needed.
