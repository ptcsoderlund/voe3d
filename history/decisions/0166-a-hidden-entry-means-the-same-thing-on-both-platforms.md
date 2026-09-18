# 0166 A hidden entry means the same thing on both platforms

Status: accepted
Date: 2026-09-17

Spec 004's acceptance ran `cmake -P check.cmake` on Windows and `platform/folder` failed on one
check: the test's `.dotted` file came back with `hidden` false. It is not a backend bug in the
usual sense — the Windows half does exactly what ADR-0162 wrote, *"a leading `.` on Linux, the
hidden attribute on Windows"* — but the two are not the same convention, and the editor's file
browser hides what `hidden` marks. Under that definition the same tree shows `.git`, `.config`
and `.ssh` in the browser on Windows and hides them on Linux, which is the engine behaving as two
different products to one person.

## Decision

**`hidden` is one meaning, the same on both platforms: a name that begins with `.` is hidden, and
on Windows `FILE_ATTRIBUTE_HIDDEN` marks one as well.** The dot rule is the engine's own
convention and is not delegated to the operating system; the attribute is what Windows adds on top
of it, because a person who marked a folder hidden there expects it hidden here. Linux has only
the dot rule, so nothing about the Linux answer changes.

**The flag is the whole answer, and no caller second-guesses it.** A reader of a listing writes
`entry->hidden`, never `entry->hidden || entry->name[0] == '.'`.

**`platform/folder.h` says this once**, and `platform/tests/folder.c` checks the dot rule
unconditionally on both platforms — that is what makes the promise one promise — with the
attribute half checked in a `#ifdef _WIN32` block.

**This amends ADR-0162, which is not edited and stays accepted.** Only its parenthetical on what
`hidden` means is replaced; folders and paths living in `platform`, the two flags on an entry, the
sorting and the atomic write all stand as written. That is the shape ADR-0047 used for ADR-0005.

## Rejected

- **Leave the per-platform meaning and make the test's expectation platform-conditional** — it
  writes the inconsistency down a second time and calls it settled. The failing check is the
  engine noticing a real difference a person would meet in the browser; silencing the check hides
  it until someone opens the same project tree on both machines.
- **Drop the dot rule and use the Windows attribute alone there** — it is the option the test
  failed on, and it means the browser offering `.git` and `.cache` as places to save a project on
  Windows only.
- **Set `FILE_ATTRIBUTE_HIDDEN` on dot names as the engine writes them** — the engine would be
  editing files' attributes to make a query answer correctly, and it says nothing about the dot
  folders everything else on the machine already made.
- **Filter hidden entries out in the editor with its own dot check** — the flag then does not mean
  what it says, and the next caller writes the same two-term test. ADR-0162 already decided the
  listing hands back flags rather than a filtered view.
- **Count `FILE_ATTRIBUTE_SYSTEM` as hidden too** — rule 10: nothing has met one. It is a
  one-term change on the day something does.

## Consequences

- The Linux answer is byte-for-byte what it was; the Linux `check.cmake` cannot show this fix
  working or failing, and the Windows run the sponsor makes next is its verification (ADR-0130).
- Windows listings mark two kinds of entry hidden, so a Windows `.gitignore` or `.voe3d`-style dot
  name stops appearing in the browser and in anything else that reads the flag.
- Something that genuinely wants Windows' own idea of hidden would need a second flag. Nothing
  does, and one is not added in advance.
