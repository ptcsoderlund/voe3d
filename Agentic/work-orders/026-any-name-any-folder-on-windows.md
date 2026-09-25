# 026 — Any name, any folder, on Windows

## What
On Windows, the editor and a game it builds work the same whatever characters are in a folder
name, a file name or the Windows user's name: Swedish å ä ö Å Ä Ö, Polish, Cyrillic, Chinese,
anything. Nothing crashes, nothing is saved to or read from the wrong place, and nothing reports
that a file cannot be found when it is there. Linux keeps working as it does. See decision 0247.

## Why
A developer must not ship a game that breaks at a customer's machine because of a name or a
folder in their language.

## How to test
On Windows:
1. Create a folder `C:\Test\Åsa 李 värld` and make a new project in it from the editor. The editor
   opens it with no error.
2. Add an entity, save the scene as `Min scen`, close the editor, open it again and open the
   project and the scene. The entity is there.
3. Press Play. The game builds and its window opens; Stop closes it.
4. Resize a panel and change the text size, close the editor and start it again. Both are as you
   left them (the editor's settings live under your user folder).
5. Create a Windows user named `Östen 张` (or use any account whose name has å/ä/ö or a
   non-Latin character), sign in as that user and repeat steps 2 and 4. Everything is kept and
   nothing fails.
6. Copy `examples\capsule` into the folder from step 1 and open it. Refresh builds its code and
   the capsule moves with the keys when you press Play.
