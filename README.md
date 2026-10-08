# <picture><source media="(prefers-color-scheme: dark)" srcset=".github/icon-dark.svg"><img src=".github/icon-light.svg" width="40" height="40" alt="" align="top"></picture> MuTap-Max

[![CI](https://github.com/tap/MuTap-Max/actions/workflows/ci.yml/badge.svg)](https://github.com/tap/MuTap-Max/actions/workflows/ci.yml)

Max/MSP externals for adaptive audio cleaning — acoustic feedback (howling)
suppression and, later, echo cancellation — built as thin wrappers over the
**MuTap** library (`tap::mu::pem_afc` and friends). A Cycling '74 Min-DevKit
package: one external per folder under `source/projects/`.

The product plan lives in the MuTap library repo:
[**`HANDOFF.md`**](https://github.com/tap/MuTap/blob/main/HANDOFF.md) (milestone
sequence, paper list, and the portability story — the same DSP core targets
Max/MSP, ARM Cortex-M55 and Qualcomm Hexagon). This package is its milestone
M5: scaffold + first external.

## Status

Early scaffold. Two objects so far:

- **`mutap.afc~`** — acoustic feedback canceller. Wraps
  `tap::mu::pem_afc<double>` (FDAF-PEM-AFROW; default speech near-end
  predictor). Inlet 1 (signal): the **microphone** signal `y`; inlet 2
  (signal): the **loudspeaker/reference** signal `u` — the same signal your
  patch sends to the speaker. Outlet 1 (signal): the cleaned signal
  `e = y − F̂·u` (times the guard's gain with `@guard 1`, below). Outlet 2
  (signal): the guard's **post-reverb gain**, linear — exactly 1 with the
  guard off. Outlet 3 (float): the **IPC** double-talk indicator, 0..1,
  reported every few processed blocks (low = near-end speech dominates and
  adaptation is being gated; high = feedback dominates the error and the
  update is informative). Outlet 4 (list): two raw **convergence
  statistics**, `uncertainty_db shadow_ratio_db`, on the same cadence, each
  `10·log10(max(x, 1e-12))` of a linear ratio. `uncertainty_db` is the
  Kalman core's identification progress, ΣP / ΣP at reset (0 dB = nothing
  identified, falling as the path is learned; it reads 0 dB with the NLMS
  core, which keeps no uncertainty — the conservative reading).
  `shadow_ratio_db` is the main filter's residual power over the shadow
  comparator's (see `shadow`): below 0 dB while the long filter out-cancels
  the short fast one, rising toward and past 0 dB when the path moves; it
  reads 0 dB with `@shadow 0`. Without the guard the external applies **no
  thresholds** — that is the patch's policy; MuTap's `pem_afc.h` and `fd_kalman.h` record
  what each statistic was measured to catch and miss, and the help patcher
  carries an example policy with its calibration conditions. Creation arg:
  feedback-path filter length in samples (default 2048);
  `partitions = ceil(filter_length / block)`.
  Attributes: `block` (canceller block size, default 256, power of two —
  changing it rebuilds the canceller), `mu` (NLMS step size in (0, 2),
  default 0.5, applied live), `adapt` (freeze/resume adaptation, default
  on), `gate` (the M4 robustness layer: IPC² step scaling + transient freeze
  ratio 4, default on — changing it rebuilds), `warp` (near-end model:
  off = the speech cascade, on = the frequency-warped LP for music/tonal
  sources — sustained low chords whose packed bass partials defeat the
  speech model; with the classic engine, warp keeps the IPC step scaling on
  regardless of `gate` because the warped whitener requires it for
  room-robust stability — changing it rebuilds), `kalman` (adaptive engine:
  off = the classic NLMS update, on = the frequency-domain Kalman filter,
  MuTap's v2 core — `mu` is ignored, `gate` selects the burst floor
  instead, the IPC outlet reports 0, and the warped model needs no IPC
  pairing — changing it rebuilds), `shadow` (shadow-comparator partitions,
  default 2, 0 = off; kept as requested, 0–4096, and clamped to the
  partition count each time the canceller is built: a small fast Kalman
  canceller adapting beside the main one on the same prewhitened signals,
  about 1 % of the canceller's cost as measured in MuTap at block 64 /
  48 kHz / 1024 taps; its power smoothing is rescaled to keep a 51 ms time
  constant at any `block` and sample rate, so a sample-rate change rebuilds
  while it is on; it applies to both engines but was measured with the
  Kalman engine only, and it does not change the cleaned output — changing
  it rebuilds). `reset` message zeroes the
  learned filter (and returns the guard to ARMING). **Adds exactly `block`
  samples of latency** on the cleaned output (the block-processing hop),
  independent of the host vector size; the gain outlet is aligned with it.

  **The safety layer, `@guard 1`** (default 0 — off, and then the object
  is exactly what it was without it: the cleaned output is bit-identical
  and the gain outlet reads 1). It runs MuTap's `tap::mu::howl_guard<double>`
  for this one microphone, fed every block with the canceller's own
  residual and its two statistics, and applies its gain to outlet 1 — the
  single-mic equivalent of `afc_chain::set_guard()`; MuTap's
  `include/mutap/howl_guard.h` lists every state and transition, and
  `docs/howl-guard.md` what was measured. In short: from every (re)build the
  output sits `@arming` dB down (ARMING) until the verdict —
  `shadow_ratio_db < @d_db` and `uncertainty_db < @a_db` — has held `@hold`
  seconds with the howl detector quiet; then it opens. A howl trip (or a
  lost verdict) ducks it `@duck` dB; it releases on the verdict, or on a
  re-arm timer (`@rearm` s × 2^strikes); each strike lowers the restore
  level 3 dB, and three latch. After 10 s in ARMING without a verdict it
  flags **unprotected**; only a cap opens it then. The guard needs both
  statistics, as `afc_chain` does: **`@kalman 1` and `@shadow` > 0** —
  otherwise the object posts an error (once DSP has started) and runs
  unguarded. Changing `@guard` rebuilds the canceller like `@block`, and a
  rebuild starts the guard in ARMING.
  Attributes (all applied live — the audio thread picks the new policy up
  at the top of the next vector; the main thread never touches a running
  guard): `cap` (the deployment gain cap in dB, ≤ 0 — MuTap's protocol
  sets the dry limit − 6 dB — or `none`, the default: with no cap a guard
  that never sees a verdict stays in ARMING, `@arming` dB down, for as
  long as that lasts, flagged unprotected; with one it opens to the cap,
  OPEN_CAPPED), `d_db` / `a_db` (the verdict's thresholds, factory
  −1.235 / −23.842 dB; setting either replaces a soundcheck calibration),
  `duck` (20 dB), `hold` (release hold, 1.5 s), `arming` (30 dB), `rearm`
  (5 s), `cal_s` (soundcheck window, 30 s), `cal_margin_d` / `cal_margin_a`
  (4 / 3 dB). Messages: `calibrate` starts the soundcheck — the guard
  samples both statistics for `@cal_s`, then runs on median + margin and
  outlet 4 sends `calibrate_done d_db a_db` (`calibrate 0` ends it early;
  store the two numbers and set `@d_db` / `@a_db` to restore a soundcheck
  later — a rebuild drops a calibration); `clear` clears the back-off
  (strikes, restore level, latch); `reset` also re-arms. With the guard on,
  outlet 4's list grows to `uncertainty_db shadow_ratio_db guard_state
  gain_db unprotected strikes`, where `guard_state` is a symbol (`arming`,
  `open`, `open_capped`, `ducked`, `releasing`, `latched` — route it with
  `sel`), `gain_db` the guard's gain at the end of the last block,
  `unprotected` 0/1 and `strikes` an int. Outlet 2 is the library's **bus
  stage** for a patch with a reverb after the object: 1 except while the
  guard is ducked or releasing, where it is the duck relative to the
  restore level — multiply the reverb's output by it (`*~`) and a trip cuts
  a charged reverb tail too (the per-mic gain is already on outlet 1, so
  ARMING and strike levels are not applied twice).
  **Limitation: one object is one mic, and two objects duck
  independently.** The library's multi-mic attribution (which mic a shared
  howl belongs to, the duck-all fallback) lives in `afc_chain` and has no
  equivalent here; with several objects summed into one bus, each guard
  sees only its own residual. To repeat the library's bus stage over
  several mics, feed the reverb's output through `*~` by the `minimum~` of
  their gain outlets (the library takes the deepest duck among the mics).
  Two of the guard's howl-detector settings are deployment calibrations and
  are attributes: `ceiling` (the absolute ceiling on the residual's block
  RMS, dB re 1.0, −120..0, default −6, MuTap's: a block at or above it trips
  in every state — set it with `cap` from the soundcheck's gain structure,
  between the programme's peak and the limiter) and `loop_ms` (the loop
  period the detector measures growth per pass against, 1–1000 ms, default
  10, MuTap's: one trip round the loop — this object's `@block`, Max's I/O
  buffers and the converters, plus the acoustic flight; at `@block` 256 in
  Max the loop is longer than 10 ms, so set it from your patch's delay
  budget). **They are detector configuration, which the guard cannot change
  live: changing either while `@guard` is on rebuilds the canceller** (the
  learned filter resets) and starts the guard in ARMING, like `@guard`
  itself; set them before turning the guard on. The detector keeps MuTap's
  defaults otherwise, except where the geometry forces a change: its fit
  window spans at least 3 blocks (from `@block` 512 at 48 kHz) and its top
  band stays under 0.45 × the sample rate (below 35.6 kHz). The guard's measurements in MuTap are at block 64,
  48 kHz, 1024 taps; this object defaults to block 256 and 2048 taps.

- **`mutap.aec~`** — acoustic **echo** canceller: the open-loop cousin of
  `mutap.afc~`, for the case where a clean far-end reference exists (the
  signal your patch sends to the speaker). Same engines, same attributes,
  same latency contract as `mutap.afc~`; what changes is the semantics —
  inlet 2 takes the **far-end** signal `x`, and the estimate cannot feed
  back around, so the hard case is **double-talk** (the near-end talker
  speaking over the echo). The PEM prewhitening handles it without a
  double-talk detector, and `@kalman 1` is the measured double-talk winner
  in the MuTap test suite (`tests/test_aec.cpp`: at 0 dB double-talk the
  naive update is kicked past useless while the Kalman core holds a deep
  estimate with ~13 dB echo suppression through the segment, config-free).
  `@postfilter 1` engages the full measured **AEC chain** — the raw Kalman
  canceller plus MuTap's coherence-driven residual suppressor, comfort
  noise matched to the room's noise floor, and the initial receive guard:
  the configuration MuTap's ITU-T compliance battery certifies at 48 and
  16 kHz (`docs/itu-compliance.md` in MuTap), its time constants rescaled
  for the actual block size and sample rate (`@comfort 0` disables the
  fill; one extra block of latency; `@mu`/`@warp`/`@kalman` are ignored
  and `@gate` selects the receive guard). The help patcher demonstrates
  fully in-patch — a delay+lowpass chain stands in for the room, so no
  acoustic loop is needed.

### How feedback cancellation works (one paragraph)

A mic and a speaker in the same room form a closed loop: the speaker signal
`u` leaks back into the mic through the room's feedback path `F`, and past a
critical gain the loop howls. `mutap.afc~` adaptively identifies `F` and
subtracts the estimate, `e = y − F̂·u`, which buys added stable gain before
howling onset. The catch is that in a closed loop `u` is *correlated* with the
near-end source (it IS the amplified near-end source), so a naive adaptive
filter converges to a biased estimate that cancels program material instead of
feedback. PEM prewhitening removes that bias: each block, the near-end signal
is modeled as shaped noise (short-term LPC + pitch predictor), both `u` and
`y` are prewhitened by the inverse model, and the filter adapts on the
whitened pair while cancellation runs on the raw signals (FDAF-PEM-AFROW;
Gil-Cacho et al. 2014, Rombouts et al. 2007). See the MuTap repo's
[`HANDOFF.md`](https://github.com/tap/MuTap/blob/main/HANDOFF.md) for the full
plan and paper list.

## Layout

```
MuTap-Max/
├── CMakeLists.txt              package build (min-devkit convention)
├── package-info.json
├── source/
│   ├── min-api/    → Cycling '74 min-api   (git submodule)
│   └── projects/<object>/      one external each (.cpp + CMakeLists.txt)
├── submodules/
│   └── MuTap/      → the MuTap DSP core    (git submodule)
├── help/                       one .maxhelp per object
├── docs/                       one .maxref.xml per object
└── externals/                  built .mxo bundles
```

MuTap is found at `submodules/MuTap` by default; override with
`-DMuTap_ROOT=/path/to/MuTap` to build against a sibling checkout.

## Build

```bash
git clone --recursive https://github.com/tap/MuTap-Max.git
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
# externals land in externals/ (e.g. mutap.afc_tilde.mxo → object mutap.afc~)
```

To use the objects in Max, make Max see this package — symlink (or copy) the
`MuTap-Max` folder into `~/Documents/Max 9/Packages/`:

```bash
ln -s "$PWD" ~/Documents/Max\ 9/Packages/MuTap-Max
```

Then the externals load and `help/<object>.maxhelp` opens from each object.

## Continuous integration

`.github/workflows/ci.yml` builds the externals universal on macOS (checking
they came out fat) and on Windows. Both this repo and the MuTap submodule are
public, so the recursive checkout needs no token.
`.github/workflows/style.yml` enforces the shared Tap House Rules
(clang-format, clang-tidy naming/braces, and drift checks against the
canonical TapHouse configs).

## Third-party code

This package's own code is MIT (`LICENSE`). The externals also compile in
third-party code from the submodules, all of it header-only:

- **No Ooura FFT code.** The real FFT the externals run (via MuTap) is DspTap's
  srdif engine, written from the published literature under a clean-room
  procedure and MIT (tap/DspTap#42). Externals built from this repository's
  trees before the srdif bump compiled DspTap's C++20 port of Takuya Ooura's
  `rdft` (a derivative work marked `SPDX-License-Identifier: LicenseRef-Ooura AND
  MIT`); the notices file records that history and keeps Ooura's notice for
  them.
- **readerwriterqueue** (Cameron Desrochers; Simplified BSD, with a zlib part in
  `atomicops.h`), which min-api's `fifo<>` is built on.
- **Murmur3** (MIT), min-api's constexpr symbol hash.
- **min-api** (the Min-API Authors; MIT) and the **Max SDK** headers it carries
  (Cycling '74; MIT).
- **MuTap** and **DspTap** themselves (MIT).

[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) holds each notice verbatim,
with the file it comes from; redistributions of the built externals should
include it. For the Ooura history, DspTap's `NOTICE.md` is the canonical
statement and MuTap's `THIRD_PARTY_NOTICES.md` carries it forward.

## Roadmap

`HANDOFF.md` in the MuTap library repo is the authority on what gets built
next. Status of this repo against it:

- **The anti-howl safety layer landed in `mutap.afc~` as `@guard`** (MuTap's
  `howl_guard`, phase 2 item 6b), one mic per object; not yet exercised in a
  running Max either.
- **M5 (this repo: scaffold + `mutap.afc~`) — code complete.** The external
  wraps the M4 processor (`pem_afc` with IPC gating and the transient freeze)
  and has **not yet been exercised in a running Max** — the help patcher's
  live mic→speaker loop is also the in-Max verification checklist (added
  stable gain by ear, IPC metering, `adapt`/`gate` A/B).
- Predictor selection landed as the `@warp` attribute (the core's
  frequency-warped music/tonal near-end model, paired with the IPC step
  scaling it requires).
- **Echo cancellation landed as `mutap.aec~`** (Stage 2 of the HANDOFF's
  "next effort"): the same engine matrix run open loop, with the measured
  double-talk behavior pinned in MuTap's `tests/test_aec.cpp`.

Note the externals compile as **C++20** (MuTap requires it). `min-posttarget.cmake`
pins each external, and min-api's test harness each `_test` target, to C++17; the
root `CMakeLists.txt` raises `CXX_STANDARD` back to 20 on every target an object
folder creates, in one loop after that folder is added.
