# MuTap-Max

[![CI](https://github.com/tap/MuTap-Max/actions/workflows/ci.yml/badge.svg)](https://github.com/tap/MuTap-Max/actions/workflows/ci.yml)

Max/MSP externals for adaptive audio cleaning — acoustic feedback (howling)
suppression and, later, echo cancellation — built as thin wrappers over the
**MuTap** library (`mutap::pem_afc` and friends). A Cycling '74 Min-DevKit
package: one external per folder under `source/projects/`.

The product plan lives in the MuTap library repo:
[**`HANDOFF.md`**](https://github.com/tap/MuTap/blob/main/HANDOFF.md) (milestone
sequence, paper list, and the portability story — the same DSP core targets
Max/MSP, ARM Cortex-M55 and Qualcomm Hexagon). This package is its milestone
M5: scaffold + first external.

## Status

Early scaffold. Two objects so far:

- **`mutap.afc~`** — acoustic feedback canceller. Wraps
  `mutap::pem_afc<double>` (FDAF-PEM-AFROW; default speech near-end
  predictor). Inlet 1 (signal): the **microphone** signal `y`; inlet 2
  (signal): the **loudspeaker/reference** signal `u` — the same signal your
  patch sends to the speaker. Outlet 1 (signal): the cleaned signal
  `e = y − F̂·u`. Outlet 2 (float): the **IPC** double-talk indicator, 0..1,
  reported every few processed blocks (low = near-end speech dominates and
  adaptation is being gated; high = feedback dominates the error and the
  update is informative). Creation arg: feedback-path filter length in
  samples (default 2048); `partitions = ceil(filter_length / block)`.
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
  pairing — changing it rebuilds). `reset` message zeroes the
  learned filter. **Adds exactly `block` samples of latency** on the cleaned
  output (the block-processing hop), independent of the host vector size.

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
code from its submodules, each under its own terms:

- **MuTap** (`submodules/MuTap`, MIT) and, through it, **DspTap**
  (`submodules/MuTap/submodules/dsptap`, MIT). Both are header-only; no
  library is built for either. One part of DspTap is not plain MIT: its real
  FFT, `include/tap/dsp/fft/split_radix.h`, is a C++20 port of Takuya Ooura's
  General Purpose FFT Package — a derivative work, not the original package,
  marked `SPDX-License-Identifier: LicenseRef-Ooura AND MIT` — and it is
  compiled into every external. Its governing notice:

  > Copyright(C) 1996-2001 Takuya OOURA
  > (email: ooura@mmm.t.u-tokyo.ac.jp,
  > download: http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html)
  > You may use, copy, modify this code for any purpose and without fee.
  > You may distribute this ORIGINAL package.

  DspTap's `NOTICE.md` is the canonical statement of how the port is
  redistributed under that notice, and MuTap's `THIRD_PARTY_NOTICES.md`
  carries it forward. Ooura's C itself is not compiled into anything this
  package builds.
- **min-api** (`source/min-api`, MIT, `License.md` there) and the Max SDK
  headers it carries (`source/min-api/max-sdk-base`, Cycling '74, MIT-style,
  `LICENSE.md` there).

Redistributions of the built externals should carry these notices forward.

## Roadmap

`HANDOFF.md` in the MuTap library repo is the authority on what gets built
next. Status of this repo against it:

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

Note the externals compile as **C++20** (MuTap requires it); each project's
CMakeLists re-raises `CXX_STANDARD` after `min-posttarget.cmake` pins it to 17.
