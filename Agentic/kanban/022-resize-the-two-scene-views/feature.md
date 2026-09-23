# 022 — Resize the two scene views

## What
The border between the two scene views can be dragged, like the panel borders from 021, to give one view
more room than the other. The pointer shows it is over a border you can drag. Neither view can be dragged
smaller than a minimum that keeps it usable. The views keep their share when the window changes size: set
them 70/30 and they stay 70/30. Double-clicking the border puts them back to an even split. The split is
remembered in the editor's own settings with the other panel sizes, so every project opens with it as you
last left it.

## Why
Decision 0228. One view often matters more than the other for the task at hand.

## How to test
1. Open the editor. Hover the border between the two scene views; the pointer changes.
2. Drag it right. The left view widens and the right narrows as you drag; both keep drawing the scene.
3. Drag it far left, then far right. It stops at a minimum each way and the smaller view is still usable.
4. Leave it at about 70/30 and make the window wider and narrower. The views stay about 70/30.
5. Drag the Scene list's or the Inspector's border. The two views share what is left, still about 70/30.
6. Double-click the border between the views. They return to an even split; the side panels stay.
7. Set it off-centre, close and reopen the editor. The split is as you left it.
8. Open another project. The split is the same there.
