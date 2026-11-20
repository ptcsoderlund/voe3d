## Assumed
- Card 21 also guards `assets/tests/keys.c:75–77` (check `row->default_text != NULL`, then a read of `[0]`), though the analyser does not report it: it is in the same folder and has the same defect.

## Learned
- `authoring/tests/reference.c:64–67, :86, :90` checks `out.text != NULL` and then passes `out.text` to `strstr` anyway. The analyser is silent about it and it is not fixed in 116.
