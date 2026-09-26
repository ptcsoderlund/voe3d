# 01 — The README opens with the name and the logo
folder: .
decisions: 0168, 0263

## Change
Only `README.md` at the repository root changes. `engine_assets/` is read-only
(decision 0263): link to its file, never copy, move or edit anything in it.

- Line 1 becomes the heading `# Voluntary Overtime Engine 3D`, replacing `# VOE3D`.
  It stays the only `#` heading; the `##` headings are unchanged.
- Directly under the heading (one blank line between), an image of
  `engine_assets/Engine images/Primary.png`, linked in place with a relative path;
  write the space in the folder name as `%20` so GitHub renders it. Alt text names
  the engine.
- Every other "VOE3D" in the text (today only the License section's opening word)
  becomes `voe3d`, lowercase. Existing file, folder and target names such as
  `voe_editor` keep their spelling.
- Nothing else in the README changes wording.

## Done when
All run from the repository root and exit 0:
- `head -1 README.md | grep -qx '# Voluntary Overtime Engine 3D'`
- `[ "$(grep -c '^# ' README.md)" = 1 ]`
- `sed -n 2,4p README.md | grep -q 'engine_assets/Engine%20images/Primary.png'`
- `! grep -q 'VOE3D' README.md`
- `grep -q '^voe3d is released under the MIT License' README.md`
- `git diff --quiet HEAD -- engine_assets && [ -z "$(git status --porcelain engine_assets)" ]`
- `git diff --stat HEAD` lists `README.md` as the only changed file outside `Agentic/`.
- `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.

For the human: preview `README.md` (or the GitHub front page) and see the heading
first with the logo on its yellow bar directly beneath it.
