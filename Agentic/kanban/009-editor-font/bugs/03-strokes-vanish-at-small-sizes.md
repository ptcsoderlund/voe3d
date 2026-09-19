# 03 — Strokes vanish from letters in a small window

## Seen
"I made the window smaller, text is not blurry now. It just scales wrong and is missing pixels.
Compare it to clion which is in background." And: "all pixels have disappeared in the 'T' in
'Theme's own'."

Seen with Oxanium chosen, in the Preferences font list, in a smaller window on the sponsor's Fedora
KDE Plasma desktop. Oxanium is sharper than Pixel Operator but letters are uneven: strokes differ in
thickness, some lose pixels, and the "T" of "Theme's own" has lost all of them. "Close" and
"Oxanium (in force)" show missing pixels too. CLion's text beside it is whole and even.

## Expected
- Per decision 0182: at every window height, every stroke of every letter keeps at least one screen
  pixel. No letter, and no part of a letter, disappears.
- A stroke coming out one screen pixel wider than another is accepted (bug 01); a missing stroke is not.
- Text still scales smoothly with the window and edges stay hard, with no grey smear.
- Holds for both Oxanium and Pixel Operator.
- Full-screen text looks no worse than it does now.

## How to reproduce
1. Start the editor and choose Oxanium in Preferences.
2. Make the window smaller in steps and look at the Preferences font list.
3. At some heights letters lose pixels; the "T" in "Theme's own" vanishes entirely.
4. Choose Pixel Operator and repeat.
