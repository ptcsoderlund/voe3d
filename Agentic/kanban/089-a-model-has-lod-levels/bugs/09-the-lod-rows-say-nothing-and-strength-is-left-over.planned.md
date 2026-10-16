# 09 — The LOD rows say nothing, and Strength is left over

## Seen
In "LODs of <file>" a row reads like "LOD 2 → 50% → ×4" with no headers and no explanation, so I cannot
tell what the numbers mean or which way they work. Every LOD showed ×4: the stable's `.parts` still held the
old `swap = farther`, which reads as ×4 for every LOD without a `swap<N>` of its own. Nothing in the panel says
so. The Strength buttons are still there, but since each LOD has its own share they no longer do anything I
can use.

## Expected
- The LOD rows have column headers, such as "Triangles kept" over the share, "Swap distance" over the swap
  and "Triangles" over the count.
- One short line under the rows explains the swap: ×2 swaps the LOD in twice as far away and looks better,
  ×0.5 twice as close and saves triangles.
- The Strength row is gone from the question. This changes decision 0435 point 1: the sponsor removes
  Strength. The edge softening and collapse limit that Strength still set become fixed at Medium's values.
  Old `.parts` files with a `strength` key still read, and the key is ignored.

## How to reproduce
1. Make LODs on the stable. The rows have no headers, every swap shows ×4, and Strength is offered.

## How to test
1. Make LODs on the stable. Each column has a header and the swap line is under the rows. There is no
   Strength row.
2. Make them with the defaults. The LODs look and drop as they did with Strength on Medium.
3. Open a model whose `.parts` still says `strength = "gentle"`. It opens without an error, and Make writes
   the file back without a `strength` key.
