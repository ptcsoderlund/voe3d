# 0048. Line endings are LF, in the repository and in the working tree, on both platforms

- **Status:** Accepted
- **Date:** 2026-09-01
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

Every text file in the planning repository showed as modified against
`origin/main` with an equal insertion and deletion count — 1342 each in
`STATUS.md` and the decision register alone. No content had changed. The
committed blobs are LF and the working tree is CRLF, so `git diff` reports every
line of every file.

The cost is not cosmetic. It makes a real change unreadable in a diff, and this
is a repository whose entire purpose is a reviewable written record. The
principal reviews by reading diffs; a diff that always shows everything shows
nothing.

**The decisive fact about this project's layout:** `/mnt/dev` is
`C:\Users\PerSoderlund\Dev` mounted into WSL over 9p. There is **one working
tree**, on a Windows filesystem, read and written by Windows editors and by
Linux agents at the same time. Windows tools write CRLF; the agents write LF.

Also fixed by earlier decisions: there is no CI (ADR-0004), so nothing but a
local rule can enforce this; and the engine is a separate repository (ADR-0002),
so it needs its own copy of whatever is decided here.

## Options considered

### Option A — `* text=auto eol=lf`
Normalise to LF in the repository, and check out LF in the working tree on every
platform. One set of bytes, one answer, regardless of which OS is looking.

Costs: a Windows editor must be willing to open and save LF files. Every current
one is — VS Code, CLion, Visual Studio and Notepad++ all default to preserving
existing endings.

### Option B — `* text=auto` alone
The standard advice: normalise to LF in the repository, check out *native*
endings in the working tree — CRLF on Windows, LF on Linux.

Costs: **it does not work in this layout.** "Native" is a property of the
checkout, and there is one checkout being read by two operating systems. Git
would check out CRLF (it is a Windows filesystem, driven by Git for Windows) and
every agent-written file would still fight it. This is the standard answer to a
different problem than the one we have.

### Option C — leave it, and normalise nothing
Costs: the churn is permanent, and it is load-bearing churn — see Context.

## Decision

**Option A: `* text=auto eol=lf`, in both repositories.** The deciding factor is
that a single shared working tree has no native line ending, so the only stable
answer is an explicit one.

## Blast radius

**Cheap, and cheap to reverse** — one file, one line that matters, and reversing
it is deleting it plus one renormalising commit. Marked cheap deliberately: this
is exactly the class of decision ADR-0006's ordering says to make fast.

The one thing that is *not* cheap is the transition, and only because it happens
once: the normalising commit touches every text file in both repositories, so it
must be its own commit or it hides real work inside itself.

## Consequences

- `git add --renormalize .` is required once per repository, and it produces a
  commit that touches nearly every file. Unavoidable, and the last one of its
  kind.
- **The engine repository needs the same file**, and it does not have one. It is
  the repository where this matters most: a C source file is read by `clang` on
  both platforms and by every agent. Handed to the principal rather than done
  from this root, per `CLAUDE.md`'s rule that engine files are not edited from
  the planning root.
- Windows-native tools that cannot handle LF would break. None in use here can't.
- Files written by agents and files written by the principal are now
  byte-identical in style, which means a diff finally means what it says.
- Binary types are listed explicitly as well as relying on `text=auto`'s
  detection, because the case detection gets wrong is a file that is mostly text
  and partly not.

## Rejected options and why

**Option B** is the answer nearly every guide gives, and it is wrong *here* for
one specific reason worth recording so nobody "corrects" it back later: it
assumes one checkout per operating system. This project has one checkout for
both.

**Option C** was rejected because the churn is not noise on the side — it
disables review, which is the mechanism this repository exists to serve.

## Questions this opens

None. One unrelated thing was noticed while writing this and is not in scope: a
compiled Linux executable, `spikes/vulkan-without-sdk/probe`, is tracked. It is
build output in a repository that builds nothing (ADR-0002). Flagged for the
principal, not fixed.
