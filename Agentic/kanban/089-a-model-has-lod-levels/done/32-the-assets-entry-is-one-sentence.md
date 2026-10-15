# 32 — The Assets panel's index entry fits its cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the entry for `interface_assets.c` is 315 characters,
cap 300. Cut it to one sentence under 260 characters: the Assets panel's
requests and menu rows carried out, Make levels asking the prepare, and the
Delete question answered. Any detail dropped (Import textures reading the
tables again, a made landscape's path, the Landscape panel opened, a material
opened in the Inspector, the open material following) must be in
`editor/src/interface_assets.c`'s header comment; it already names most of
them, add any it does not, keeping the header within its cap. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `editor/src/`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines editor/src/interface_assets.c`
prints no "over cap".
