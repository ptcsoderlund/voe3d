# 02 — I cannot re-import a model

## Seen
"We want to do "reimport" for glb file when right clicking. To recreate material and make model ready for the
engine." My models were imported before, and there is no way to redo a `.glb` already in Assets short of
importing a copy.

## Expected
Decision 0429. Right-clicking a `.glb` in Assets offers **Re-import**, and the `.glb`'s Inspector has a
Re-import button too. It reads the file again under the splash ("Preparing <file>"): its materials and parts
are written afresh from the file (my edits to those materials are lost, the PNGs are not touched), and its
levels are made again with the model's level settings, which stay as they were. Every placed copy shows the
result without a restart.

## How to reproduce
1. Open the hill project with an imported `.glb`.
2. Right-click it in Assets. There is no Re-import.

## How to test
1. Edit one of an imported model's materials (change its colour). Right-click the `.glb` → Re-import. The
   splash shows it being prepared; the material has the file's colour again, and every placed copy shows it.
2. Re-import a model whose textures I imported: the PNGs are still there.
3. Select the `.glb` in Assets and press Re-import in the Inspector: the same happens.
4. Give a model custom level settings (bug 03), Re-import it: the settings are unchanged.
