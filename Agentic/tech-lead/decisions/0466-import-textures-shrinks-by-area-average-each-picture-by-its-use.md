# 0466 — Import textures shrinks by area average, each picture by its use, after a four-button question
date: 2026-10-22
by: planner

## Decision
For 092, how 0453's largest size is carried out.

1. **The size.** A picture whose longest side is at most the chosen size is written as decoded.
   Otherwise its longest side becomes the chosen size exactly and the other side is scaled by the
   same factor, rounded, at least 1.
2. **The filter is an area average,** in `assets` beside the PNG encoder (`assets/image.h`): each
   written texel is the coverage-weighted mean of the source texels under its footprint, edge texels
   weighted by the part covered, so a non-whole ratio is exact and nothing aliases. Working memory is
   a row of floats per output row, never a float copy of the whole picture.
3. **Each picture by its use.** A picture any material names as its normal map is a normal: texels
   decoded to vectors (`2c − 1`), averaged, made unit length again, encoded back, so bumps neither
   flatten nor turn to noise. Else, one named as a base colour or emission map is colour: rgb averaged
   in linear light (sRGB decoded and encoded again), weighted by alpha so a cut-out's hidden texels do
   not bleed into its edge (plain weights where all are transparent), alpha averaged plainly. Every
   other picture, the merged ORM included, is data: each channel averaged as stored.
4. **The question.** Import textures on a `.glb`'s menu opens an anchored panel like the levels
   question: a title naming the file, a "Largest size" row of four buttons, 512, 1024, 2048 and 4096,
   the chosen one lit and 2048 on every opening (nothing remembered), then Import and Cancel. Cancel,
   Escape or a press outside writes nothing.

## Reasoning
An area average is the cheapest filter that is correct at any ratio and keeps a picture's mean; a
bilinear sample of a 4K picture at 1K skips three texels in four and shimmers. Averaging sRGB bytes
darkens contrasty colour, and averaging normals as colour shortens them, which reads as flatter
bumps; both are a few lines to get right once. Remembering the last size would make a re-run's size
depend on what was done to another model; the feature asks for 2048 already selected.

## Replaces
Nothing.
