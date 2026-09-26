# 033 — The README opens with the name and the logo

## What
When someone opens the repository, the first thing they see is the heading
**Voluntary Overtime Engine 3D**, with the logo right beneath it. After that, the README calls the
engine voe3d.

- The heading is the only `#` heading in the README and the first line of the file.
- The logo is `engine_assets/Engine images/Primary.png`, shown where it already is. Nothing is
  copied, renamed or moved, and nothing else in `engine_assets/` changes (decision 0263).
- Everywhere else in the README, the engine is called voe3d, lowercase. File, folder and target
  names that already have their own spelling (such as `voe_editor`) keep it.
- The rest of the README says what it says today. Only the top and the engine's name change.

## Why
The name and the logo should be what people meet first, and the README should use one short name
for the engine.

## How to test
1. Open the repository's front page on GitHub (or preview `README.md`): the first thing on the
   page is the heading "Voluntary Overtime Engine 3D", and directly under it is the VOE3D logo on
   its yellow bar.
2. Read on: after the heading, the engine is called voe3d, and "VOE3D" appears nowhere in the text.
3. `engine_assets/` is exactly as I left it.
