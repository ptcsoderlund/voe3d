# 09 — dev opens in step with the display
folder: dev
decisions: 0168, 0201, 0215
read: feature.md

## Change
`voe_dev` asks for MAILBOX at startup (`mailbox_wanted = true`, after card 08 in
`dev/src/startup.c`), so focused it draws flat out. By 0215 it opens on FIFO like the editor.

- `dev/src/startup.c` (and `startup.h` if the request's flag is a member there): the startup
  request for MAILBOX goes; the device's own FIFO default stands, so nothing is requested at
  startup. The key in `dev/src/main.c` that toggles MAILBOX stays and now starts from FIFO; its
  comment says a person pressing it is asking to measure the uncapped rate (0215).
- Wherever dev's header or the readout's header (`dev/src/readout.h`, after card 07) says dev opens
  uncapped or on mailbox, it says FIFO and cites 0215.

This is the feature's last card: `## How to test` in `feature.md` is made true by cards 02, 04, 05
and this one together.

## Done when
`checks.sh --folder dev` exits 0, then `checks.sh --all` exits 0.

For the human, on the laptop with both cards, per `feature.md`'s `## How to test`:
1. `voe_editor` prints one `render` line naming the NVIDIA RTX 4070, discrete, drawing in step
   with the display.
2. Focused with a scene open, `ps -o pcpu=,rss= -C voe_editor` is well under 100; note the rss.
3. Another window focused: the CPU figure is near 0; clicking back shows a correct picture at once.
4. Another virtual desktop for a minute and back: as step 3.
5. Minimised for a minute: near 0 CPU.
6. An hour of ordinary use with the editor open: the desktop stays responsive and clean, and
   `journalctl -k -b | grep -iE 'GPU HANG|Fence expiration'` shows nothing newer than the start.
7. The rss from step 2 has not climbed.
8. Closing the editor leaves the desktop as it was.
9. `voe_dev` passes steps 2 and 3 the same way, with `-C voe_dev`.
