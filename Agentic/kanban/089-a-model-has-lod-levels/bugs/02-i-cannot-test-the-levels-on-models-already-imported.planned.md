# 02 — I cannot test the levels on models already imported

## Seen
"My models were already imported, I cannot test LOD without the reimport feature."

## Expected
Decision 0429. A model imported before 089 gets its levels file the first time it is loaded, with no
re-import. Right-clicking its `.glb` in Assets offers Make levels, which makes the file again behind the
splash ("Preparing <file>") without touching the model's materials.

## How to reproduce
1. Open a project whose `.glb` was imported before 089.
2. Look for a way to have its levels made where I can see it. There is none but importing it again,
   which rewrites its materials.

## How to test
1. Right-click the `.glb` in Assets → Make levels: the splash shows "Preparing <file>" once.
2. Place it and fly away: the Inspector's Level goes up.
