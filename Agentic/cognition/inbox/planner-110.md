## Assumed
- The splash wait's thread counts under 0497's "no thread created between opening and closing", since 110's test 5 (Play, whose cook is a splash wait) expects none; recorded in 0504 point 5.
- Programs open their standing workers through `voe_app_new`/`_headless` and close them at `voe_app_destroy`, so dev and an editor `--capture` get them too.

## Learned
- No burst task starts a burst today; the only nesting is a splash wait's work starting bursts (the model batch, a material's load), which is why one level suffices.
- 0497's Reasoning line that the watch makes about four threads per model each second is stale: the watch already checks stamps first and starts no burst when nothing changed.
