# 04 — File labels in the Assets panel are too bright

## Seen
In the Assets panel, the labels of files (`.glb`, `.png` and prefabs, the only kinds there are yet) are
brighter than the text everywhere else in the editor. Folder labels in the same panel look right: they
match the other panels.

## Expected
File labels look like folder labels and like the text in every other panel. They take their colour
from the theme in force and move with the contrast slider in Preferences, as the rest of the editor's
text does.

## How to reproduce
1. Open `examples/tank_game` in the editor.
2. Look at the Assets panel: a folder, a `.glb`, a `.png` and a prefab side by side. The three file
   labels stand out brighter than the folder label and than the Inspector's text.
3. In Preferences, drag contrast all the way down, then all the way up. All four labels, files and
   folders alike, should dim and brighten together with the rest of the editor.
