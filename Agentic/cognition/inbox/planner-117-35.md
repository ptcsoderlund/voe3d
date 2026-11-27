## Assumed
- A copy keeps the turn, scale and lean drawn when painted whatever glb its group picks later; base size, shadows and blend are the picked glb's (0521).
- Adding a group or a glb and replacing a glb's path are not undo steps; removing a group or a glb is one (0521).
- A group's title is its first glb's file name and how many more it holds; groups have no name of their own (0521).
- A person's old `kind <k> <path>` folds are left unread, so every group shows open once after the change.
- A group holds at most 8 glbs, a path at most once.

## Learned
- Shares do not need storing per copy: the variation pick of card 27 was already live, keyed by the kind's seed and the copy's mm position.
- The tree maker's code spans editor (cards 08, 09, 11-17, 24), game (06, 18), 3d (02-05, 23, 27, 33) and assets (01, 22); its removal goes callers first, editor down to assets.
