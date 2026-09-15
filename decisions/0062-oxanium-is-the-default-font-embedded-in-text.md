# 0062. Oxanium is the default font, vendored in `text` and embedded

- **Status:** Accepted
- **Date:** 2026-09-04
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal asked for **Oxium** as the engine's default font, and named a
future company that should share it. **The typeface is spelled Oxanium** — square,
futuristic, seven weights, by Severin Meyer, under the **SIL Open Font License
1.1**, on Google Fonts with sources at `github.com/sevmeyer/oxanium`. No
typeface called Oxium is published; the correction is recorded here because the
next act is downloading a file and the wrong name would fetch nothing.

The OFL is the good case: commercial use permitted, and bundling and embedding
in software — including sold software — explicitly permitted, provided the
licence text travels with the font and a derivative does not reuse the reserved
name.

Constraints already fixed:

- **The planning root builds nothing** (ADR-0002, root `CLAUDE.md`). A font there
  could only be a brand asset, never engine input.
- **The fetched side of the build is empty and that is a good outcome**
  (ADR-0021, ADR-0023's closing consequence). Fetching a font at configure time
  would put a network dependency back for the least interesting reason in the
  project.
- **Shaders are compiled offline and embedded in the binary** (ADR-0046) — the
  mechanism that forced the Clang 19 floor (ADR-0047) exists and is proven.
- **Third-party dependencies default to no** (ADR-0023), and that ADR governs
  *code we could write*. A typeface is content and cannot be written; the gate
  that still applies is the principal's explicit yes, given here.
- **Implement on demand** (ADR-0034). The `text` folder does not exist yet; card
  021 creates it, and card 021 writes the TrueType reader by hand (ADR-0023).
- **Three builds — engine, editor, game** (ADR-0052), all of which would carry an
  embedded font.

## Options considered

### Option A — engine repo, embedded in the binary
One static weight vendored in the engine repo, embedded the way shaders are. The
default font is always present: no file lookup, no asset path convention, no
dependency on file I/O that `platform` does not have. Frame statistics draw on a
fresh clone with nothing installed.

### Option B — engine repo, loaded from disk at runtime
Vendored as a data file and read at startup. Needs `platform` file I/O and an
asset-path convention, neither of which exists, and makes the *default* font
something that can be absent — the one font that must never be.

### Option C — the planning root, as a brand asset as well
Option A plus a labelled copy at the root, for the company. Two copies to keep in
step, one of them serving a company that does not exist.

## Decision

**Option A.** The deciding factor: the default font is the one font that must
never be missing, and embedding is the only placement where "missing" is not a
state it can be in.

- **Oxanium is the engine's default typeface.**
- **One static weight — Regular — as a `.ttf`.** Not the variable font: card 021
  writes the outline reader by hand, and a variable font means implementing
  glyph-outline interpolation before a single letter appears. **TrueType `.ttf`,
  not `.otf`**: `glyf` outlines are quadratic, which is what card 021's stated
  non-zero winding rasteriser assumes, while an OTF carries cubic CFF outlines
  and is a different reader.
- **It lives in the engine repository, in the `text` folder, in `fonts/`** —
  beside the code that reads it, not under `src/`, so ADR-0027's globbed sources
  stay sources. `assets` is readers, not data, and takes no copy.
- **It is embedded in the binary** under ADR-0046's mechanism.
- **The OFL text is vendored beside it, unmodified, with its copyright line.** The
  file is neither renamed nor modified, so the reserved-name clause never
  engages.
- **It arrives with the card that renders it, not now.** Only the placement and
  the licence handling are decided today — the same test ADR-0054 set: placement
  changes what later code looks like, so it is worth deciding before the code
  exists.
- **The brand copy is not decided and the engine repository is not the brand's
  home.** A brand font serves documents, a site and a design system; making them
  depend on an engine checkout would be backwards. It is the same file from the
  same place whenever the company needs it.

**Not decided, and not to be assumed:** a second weight, italics, synthetic
emboldening, user-supplied fonts at runtime, and font fallback for glyphs Oxanium
does not carry. Each arrives with a card that needs it. Oxanium supports the
Adobe Latin 3 character set — anything outside it has no answer in this ADR.

## Blast radius

**Cheap on the typeface, moderate on the mechanism.** Swapping Oxanium for
another OFL font later is a file swap and a name change. What later cards will
come to rely on is the *property* — that a font is always present, so on-screen
text needs no assets and no file I/O. Reversing that turns every text call site
into one that can fail for a new reason.

## Consequences

- **The binary grows by the font**, tens of kilobytes. Unnoticeable, and paid by
  all three builds.
- **This is the first third-party content in the tree.** ADR-0023's line is
  unchanged for code; the precedent set is narrower than it looks — content that
  cannot be written, with the principal's yes, per item.
- **A shipped game must carry the OFL notice**, and the three-builds shape has no
  mechanism for surfacing third-party notices. That is a real gap opened by this
  decision, not a paperwork detail: the licence permits the embedding *provided*
  the notice travels.
- **Card 021's reader is now aimed at a known file.** It must read this file's
  tables and no others until something asks — which makes the card smaller and
  more testable, since the expected glyph outlines are fixed and checkable.
- **The dev program's statistics text has a font before it has an asset
  pipeline**, which is what keeps card 020's output honest without pulling
  file I/O forward.
- **No bold.** Anything wanting emphasis before a second weight is embedded gets
  none; faux-bold is not adopted by default.

## Rejected options and why

- **Option B — a runtime file.** The right shape for *user* fonts later, and the
  wrong shape for the default: it invents an asset-path convention that no
  decision has taken, needs file I/O that `platform` does not have, and buys a
  failure mode in exchange.
- **Option C — a brand copy at the root now.** Two copies of the same file that
  can drift, to serve a company that does not exist. The planning root builds
  nothing, so the copy would sit there unread until it was stale.
- **Fetching it at configure time.** Contradicts the outcome ADR-0021 and
  ADR-0023 arrived at together — an empty fetched side — and trades an offline
  build for sixty kilobytes.
- **The variable font.** One file for seven weights is the right answer for a
  browser and the wrong one for a hand-written rasteriser. Reconsider only if a
  card wants continuous weight, which nothing does.

## Questions this opens

- **D-072** — where the OFL and any later third-party notice surfaces in a
  shipped game and in the editor, under ADR-0052's three builds. Trigger: the
  font landing in the tree, since the obligation starts then.
- **D-073** — the company's brand font home, when the company exists. Parked
  with no consumer; the engine repository is excluded as an answer.
- **D-074** — user-supplied fonts loaded at runtime, which needs `platform` file
  I/O and an asset-path convention. Deferred; trigger: the first card that loads
  a font from disk.
- **D-075** — glyph coverage beyond Adobe Latin 3, and whether fallback is a
  concept the engine has. Deferred; trigger: the first card needing a glyph
  Oxanium lacks.
