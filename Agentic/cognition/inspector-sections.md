# Inspector sections

## What folds
- Every Inspector component section folds, and Sculpt, Layers, Foliage, Brush and LOD are sections too. [observed · coder-108-05 · 2026-10-30]
- The Enabled row, the Duplicate and Delete row, a part's line, Add component and the open material's section are not sections and never fold. [observed · accept 108 · 2026-10-30]

## How a fold is kept
- A fold is kept by section type on every entity, a foliage kind's by landscape and kind index, taken as recommended with no reason of the sponsor's own; whether a section folded on one light should stay folded on every light was asked and not answered. See 0502. [assumed · tech-lead · 2026-10-30]
- The fold store holds 128 names; when it is full a new fold is dropped unreported and the section stays open. Past 64 fold buttons in one frame a section still draws but its button is not recorded. [observed · accept 108 · 2026-10-30]
- A failed fold write is reported only as an error where it happens, never as the top bar's notice, unlike Snap's. [observed · accept 108 · 2026-10-30]

## What a folded section loses
- While Layers or Foliage is folded none of its rows is recorded, so a material or `.glb` dropped onto it lands nowhere until it is opened. [observed · coder-108-05 · 2026-10-30]

## The box
- Every section draws on the surface colour with a border; the raised shade the component, Sculpt, Layers, Foliage, Brush and LOD panels had is gone. [observed · coder-108-05 · 2026-10-30]
- A section keeps the old Inspector panels' spacing (pad 2, gap 1.5, header gap 2 mm at spacing one), its fold button and title centred vertically in the header, so a panel swapped for a section keeps its spacing. [observed · accept 108 · 2026-10-30]
- A section cannot widen itself to its parent: layout allows no grow across, so it takes its parent's width only when the parent fills across. It fills its own children, so nested sections and body rows take its width; a top-level section is as wide as its panel only if the panel fills. [observed · coder-108-03 · 2026-10-30]
- The fold button is keyed "fold" inside the box, so a title choice in the header row may not be named "fold". [observed · coder-108-03 · 2026-10-30]

## The arrow
- Oxanium has ‹ and › but no down-pointing arrow or triangle (U+2193, U+25B8, U+25BE), so a fold arrow cannot come from the glyph sheet as it stands. [observed · planner-108 · 2026-10-30]

## The budget
- The Inspector's node and element budget never counted the contents of Sculpt, Layers, Foliage and Brush, only LOD's. [observed · coder-108-05 · 2026-10-30]
