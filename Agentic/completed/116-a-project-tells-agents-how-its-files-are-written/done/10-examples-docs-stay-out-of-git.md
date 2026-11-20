# 10 — Examples' Docs stay out of git
folder: .
after: none
decisions: 0168, 0511

## Change
- `.gitignore` (the root one): add `/examples/*/Docs/`, with a comment saying the editor writes it and
  it holds machine-local paths.

## Done when
`git check-ignore -q examples/capsule/Docs/voe3d.md` exits 0.
