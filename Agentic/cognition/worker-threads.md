# Worker threads

## Who has them
- Every program made through the app has standing workers, dev and an editor `--capture` too. See 0504. [assumed · planner-110 · 2026-10-30]
- The workers open only after a startup's last step that can fail, so a failed start never leaves them standing. [assumed · coder-110-02 · 2026-10-30]
- Closing the workers while another thread starts a burst is not guarded, only stated as a constraint; the app closes them last. See 0504. [assumed · coder-110-01 · 2026-10-30]

## Nesting
- No burst task starts a burst; the only nesting is a splash wait's work starting bursts (the model batch, a material's load), which is why one level suffices. [observed · planner-110 · 2026-10-30]
- A nested burst is sure of workers only while the outer bursts whose tasks wait on nested ones are together no wider than the standing count less one: two splash-wide such bursts at once could fill every worker with waiting tasks and hang. One at a time, as the splash wait is, is safe. [inferred · coder-110-01 · 2026-10-30]
- A worker that joins a burst stays on it until no indices are left; its place is never handed back. [assumed · coder-110-01 · 2026-10-30]

## The watch
- The model watch checks stamps first and starts no burst when nothing changed; 0497's reasoning that it made about four threads per model each second is stale. [observed · planner-110 · 2026-10-30]
