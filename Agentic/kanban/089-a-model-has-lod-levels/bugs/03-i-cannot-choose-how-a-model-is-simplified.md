# 03 — I cannot choose how a model is simplified

## Seen
"I also want to have settings for calculating LODs. Decimation strength or something. So I can decide
density. When doing games for smartphones we would like to have a more relaxed setting than the pc/console
version. Or some model might not be important because they are background, maybe dev want to do heavy
decimation on their LOD." And: "I want the inspector to be a tool both for scene view and assets panel. Select
something in assets panel, its settings and tool show in inspector in the same way as entities in scene view
does. So contextual inspector depending what is selected and where."

## Expected
Decision 0429. The Inspector shows whatever I pressed last, an entity or a file in Assets, the same way. A
selected `.glb` shows its level settings: how many levels (0 to 3) and, per level, the percentage of the
model's triangles it keeps (50, 25, 12.5 until I change them). Changing them remakes that model's levels
straight away, and they are saved beside the `.glb`, so they hold after closing and in Play and Ship. Under
them I see what each level actually reached. A `.glb` with its own Blender levels says so and has no settings.

## How to reproduce
1. Import a tree and select its `.glb` in Assets.
2. The Inspector shows no level settings; every model gets 50, 25 and 12.5.

## How to test
1. Select a tree's `.glb` in Assets: the Inspector shows its levels, 3 at 50, 25, 12.5, and the triangles each
   reached. Select an entity in the scene: the Inspector shows the entity. Select the `.glb` again: back.
2. Set the tree to 2 levels at 30 and 5. The levels are remade at once; fly away from a placed tree and the
   Inspector's level goes to 2 at most; far away the Frame panel's Triangles is lower than before.
3. Set a rock to 0 levels: it always draws whole.
4. Close and reopen the project: the tree still has 2 levels at 30 and 5, and is not prepared again.
5. Press Play, then Ship and run it: the tree uses its own settings there too.
6. Select a `.glb` with its own Blender levels: the Inspector says it brings its own levels.
