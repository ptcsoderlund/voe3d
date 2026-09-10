# bug 003 — Windows runs at the display rate on mailbox, and fifo halves it to 60

status: investigated, then **parked by the principal, 2026-09-10** — *"its not a
  problem so we leave it until it is."* No card, and none is coming until the
  trigger below fires. **Comes back to the inbox the moment it does.**
archived: 2026-09-10, on the parking — the inbox holds only reports awaiting a
  decision, and this one has had its decision. **The trigger is the number in *What
  confirms or kills it*, or any Windows run where the frame rate is a complaint
  rather than a curiosity.** The measurement stays worth taking; it is simply not
  worth anybody's time today.
found-by: the principal, 2026-09-10, running `voe_dev` on Windows and on the Linux
  dev machine, same hardware
reported-by: claude-opus-5 (tech lead), from the principal's words
folder: `render` (present path), possibly `platform` (Win32 window)
severity: on Windows the frame rate readout measures the desktop compositor and not
  the engine, in both present modes — so no performance number taken on Windows means
  anything today. **Nothing is drawn wrongly**: the picture is correct, the engine is
  not slow in any way this shows, and Linux is unaffected. If the half-rate half of
  this holds it also costs a *shipped* program half its frames on a 120 Hz Windows
  display, and that is the part that is worth more than a readout.

## The report, in the reporter's words

> is windows doing some v-sync? I get 119-120 fps and if i press "p" i get 60 fps.
> On my linux dev machine i get a few hundred fps and 120 on "p". Same hardware.

## What happens

| | default (mailbox) | after `P` (fifo) |
|---|---|---|
| Windows | 119–120 /s | 60 /s |
| Linux dev machine | a few hundred /s | 120 /s |

## What should happen

`P` swaps the two present modes: the engine opens on mailbox and `P` asks for fifo
(`dev/src/main.c:2309`). Fifo waits for the display, so **120 /s on a 120 Hz display
is fifo working exactly as specified** — that is the Linux column, and it is right.
Mailbox imposes no ceiling, so the mailbox column should be whatever the machine can
actually do and should look nothing like a display rate.

So of the four numbers, **one is correct (Linux fifo), one is a real measurement of
the machine (Linux mailbox), and two are not measurements of this engine at all.**

- **Windows mailbox at 119–120** is the display rate wearing the uncapped mode's
  name. Mailbox does not settle on a round display-shaped number by coincidence.
- **Windows fifo at 60** is *half* the display rate, which fifo only does when a
  frame misses its deadline — or when something else is pacing the presents.

## Evidence that the code ran

The picture is correct in both modes on both platforms and the rate changes on the
keypress, so the mode switch reaches the driver and the swapchain is rebuilt. Nothing
here is a stale build or an unreached call.

**And the Windows half of this was measured once already, on 2026-09-08**, with the
dev program's own block rather than Task Manager. Same machine, RTX 4070 Laptop at
100 W, mailbox throughout:

| block | rate | `frame` avg | `draw` avg | `gpu` avg |
|---|---|---|---|---|
| first 3 s | 767 /s | 0.87 ms | 0.85 ms | 0.03 ms |
| after a ~1 s stall | 120 /s | 8.33 ms | 8.32 ms | 0.08–0.11 ms |

Three things are established by that table and do not need measuring again:

1. **The Windows display is 120 Hz.** 8.33 ms is 120 Hz to three figures.
2. **The card is idle.** 0.1 ms of GPU work inside an 8.33 ms frame. The engine is
   not slow on Windows; it is *waiting*.
3. **The wait is inside `draw`, which is where the acquire and the present live** —
   so it is the driver and the compositor, not our code. The loop, the fence wait, the
   acquire, the present and the swapchain setup were read at the time and contain no
   sleep and no throttle.

**What is new in this report is the fifo column.** On 2026-09-08 only mailbox was
measured, and 120 /s on mailbox is a cosmetic complaint — nobody wanted more than the
display can show. 60 /s on fifo is not cosmetic.

## Where it isolates to

Same hardware, same card, same scene, same source. **The one thing that differs is
the operating system's presentation path**, and Windows composites a windowed
swapchain through the desktop compositor unless the driver promotes the window to a
direct flip. The 2026-09-08 run caught that promotion coming and going: uncapped at
767 /s for three seconds, a one-second stall, then pinned to the display for the rest
of the run, with `_get` still honestly reporting mailbox because mailbox is genuinely
what the swapchain was created with.

## The suspect, labelled a hypothesis

**Hypothesis A — the compositor paces both modes, and composited fifo costs a whole
extra refresh.** Under composition the compositor takes one image per refresh and
inserts itself between our present and the scanout. Mailbox then reads as the refresh
rate (it cannot go faster than the compositor consumes) and fifo, which must also wait
for its own vblank on top of that, lands on every second one. This predicts `frame`
≈ 16.6 ms with `gpu` still ~0.1 ms — the program asleep, not busy.

**Hypothesis B — the frame genuinely costs a hair over 8.33 ms on Windows now.** The
scene has grown a great deal since 2026-09-08 — panels, text, sprites, the interface.
A frame that costs 8.4 ms cannot hold 120 Hz in fifo and drops to the next thing it
can hold, which is exactly 60. Mailbox would be hiding the real cost behind the
compositor's cap, so 119–120 would tell us nothing either way. This predicts a real
number in `update`, `draw` or `gpu`.

The two are not exclusive and B would be the more important of the two if it holds.

## What confirms or kills it, in one line

**Press `P` on Windows and read the four lines under the rate: `frame`, `update`,
`draw` and `gpu`.** If `frame` is ~16.6 ms while `gpu` stays near 0.1 ms and `update`
near zero, the program is asleep and it is A. If `gpu`, `update` or the non-waiting
part of `draw` is a real fraction of the frame, it is B and the engine has genuinely
become slower than one refresh.

Two extras, both free and neither blocking: what the `first 3 s` block says before the
compositor settles (that is the only honest throughput reading Windows offers today),
and whether CLion is running a **Debug** preset — a debug build attaches the Vulkan
validation layer whenever the SDK is installed (`render/src/device.c:185`), Linux
release builds do not, and that alone can account for several milliseconds a frame.
The startup banner does not print either fact, which is why this has to be asked
rather than read.

## Why it was not caught before

It was caught, on 2026-09-08, and **the principal parked it himself** in those terms:
*"we just leave it until we have some complex geometry and really need to optimize. We
accept these numbers for now."* That was the right call on the evidence then, which was
mailbox-only and cost nobody anything. The fifo half was never measured, and the
scene that the parking was waiting for has since been built.

## Verified on

- **Windows**, RTX 4070 Laptop 100 W, 120 Hz, windowed, the principal, 2026-09-10 —
  reproduces. Build configuration not recorded.
- **Linux dev machine**, same hardware per the reporter, 120 Hz — **does not
  reproduce**, and that is the finding: fifo pins to 120 and mailbox runs free. Note
  that the last recorded Linux figures were 165 Hz and 1300–2500 /s on a heavier
  compositor path, so the Linux display is being driven differently now than it was;
  worth a glance but not a fault.
- **The WSL workstation** cannot see this at all: every Linux verification in this
  project runs on a software rasteriser, and this is a presentation-path fault on a
  real driver.

## Notes, and what is *not* the fault here

- **`_get` is not lying.** It reports mailbox because the swapchain really was created
  with mailbox. What it cannot report is that something downstream is pacing the
  presents anyway, and no Vulkan call will tell it so.
- **The engine is not slow on Windows on this evidence.** 0.1 ms of GPU work is 0.1 ms
  of GPU work. Hypothesis B is the only route to *slow*, and it is unmeasured.
- **A separate finding, not this fault, turned up while reading the code and it is
  the tech lead's to settle, not a coder's:** the engine's device opens wanting
  **mailbox** (`render/src/device.c:1234`), while the decided record says the engine
  opens on **fifo** and that uncapped is a request a program makes at startup. The
  request appears to have been implemented one level too low. It changes who this bug
  hurts — with the engine defaulting to mailbox a shipped game gets the compositor's
  cap, which nobody minds; with fifo as decided it would get half the display rate —
  so it is named here and is being taken to the principal separately.

## Cards this does not impeach

Nothing on the board. Cards 041, 043 and 045 touch anchoring, sample sets and the
shader compiler check; none of them go near the present path, and none of their
verification depends on a frame rate.
