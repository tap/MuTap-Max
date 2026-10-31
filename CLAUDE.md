# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A Cycling '74 **Min-DevKit Max/MSP package**: thin wrappers exposing the **MuTap** adaptive-filtering
library (`tap::mu`) as Max objects. Two externals so far — `mutap.afc~` (acoustic feedback / howling
canceller) and `mutap.aec~` (acoustic echo canceller). One external per folder under
`source/projects/`.

**All DSP lives in `submodules/MuTap`, never here.** This repo holds Max glue only: attribute
plumbing, thread handoff, vector-size bridging. A missing kernel feature is a MuTap change plus a
submodule bump — never a local workaround. MuTap's `HANDOFF.md` is the roadmap of record; this
package is its milestone M5. `README.md` here documents every attribute's meaning and is the
user-facing contract.

Neither external has been exercised in a **running Max** yet (CI builds and Min-level tests pass).
`help/mutap.aec~.maxhelp` demonstrates fully in-patch (a delay+lowpass chain stands in for the
room); `mutap.afc~`'s help patcher needs a live mic→speaker loop and doubles as the in-Max
verification checklist.

## Build & test

```sh
git submodule update --init --recursive           # min-api (+ its own nested) and MuTap
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release    # macOS: universal arm64;x86_64 by default
cmake --build build --parallel
ctest --test-dir build --output-on-failure        # mutap.afc_tilde_test, mutap.aec_tilde_test
ctest --test-dir build -R afc --output-on-failure # one object's tests
./tests/mutap.afc_tilde_test "*block size*"       # Catch2 binary directly, by scenario name
```

Build against a sibling MuTap checkout instead of the pin: `-DMuTap_ROOT=/path/to/MuTap`
(configure hard-fails with a clear message if `include/mutap/pem_afc.h` isn't there).

To load in Max: `ln -s "$PWD" ~/Documents/Max\ 9/Packages/MuTap-Max` — the repo *is* the package.

Before pushing C++:

```sh
pre-commit install    # once per clone — pinned clang-format on staged C/C++
scripts/tidy.sh       # local mirror of the CI clang-tidy gate; needs clang-tidy-18 specifically
```

Two local facts about `scripts/tidy.sh`: the no-argument sweep needs a bash with `mapfile`
(macOS's `/bin/bash` 3.2 lacks it — pass the files you changed instead), and its output includes
naming warnings from min-api's own headers, which CI filters out (`-header-filter` keeps only
`source/projects/`); only findings in our TUs count.

`.clang-format`, `.clang-tidy`, `STYLE.md`, `.pre-commit-config.yaml`, `scripts/tidy.sh` and
`.claude/hooks/session-start.sh` are **copies synced from TapHouse** (pinned `v7` in
`.github/workflows/style.yml`). Never hand-edit them here — a drift-check failure is the mechanism
working. Naming rules that clang-tidy enforces: `snake_case` types/functions, `m_` members, `k_`
constants, PascalCase for template parameters only.

## The wrapper pattern (both externals; follow it for new ones)

Read `source/projects/mutap.afc_tilde/mutap.afc_tilde.cpp` once — `mutap.aec_tilde.cpp` is the same
skeleton with extra engines. The structure exists to satisfy Max's real-time contract:

- **`struct engine`** = one canceller (a `std::variant` over the near-end-model × core
  instantiations) plus its `x`/`y`/`e` block buffers and a `fill` cursor. Built whole on the control
  thread; the audio thread dispatches with `std::visit`, which never allocates. Engine-specific APIs
  are reached with `if constexpr (requires { ... })`, not by branching on the flags.
- **State members are declared ABOVE the attributes on purpose.** min-api invokes each custom setter
  with the default value during attribute construction, and members initialize in declaration order —
  anything a setter touches must already be alive. `m_constructed` guards `publish()` until the
  constructor body runs.
- **Two classes of attribute.** *Structural* (`block`, `gate`, `warp`, `kalman`, and on the AEC
  `postfilter`, `comfort`) take `m_control_mutex`, mutate control-side state, and call `publish()` —
  a full rebuild, so the learned filter resets. *Scalar* (`mu`, `adapt`, `reset`) travel through
  `std::atomic` and are applied by the audio thread every vector, no rebuild.
- **Lock-free handoff.** `publish()` builds the new engine and swaps it into `m_pending`; the audio
  thread adopts it at a vector boundary and parks the retired one in `m_trash`, which the control
  thread reaps at the top of the next `publish()`. The audio thread never allocates, frees, or takes
  the mutex. Adoption is skipped while `m_trash` is occupied. `publish()` swallows exceptions
  deliberately — the running engine keeps going, and with no engine at all the perform routine
  passes the dry mic signal through.
- **Vector-size bridging.** The perform routine gathers samples into the block buffers, calls
  `process_block()` when a block fills, and plays the processed block out — so the object adds
  **exactly `@block` samples of latency** whatever the host vector size is. Inputs are read before
  the output is written because Max may alias output onto input buffers.
- **The right outlet** is `outlet<thread_check::scheduler, thread_action::fifo>` and sends a
  pre-allocated `atoms` member (`m_ipc_atoms`), not a bare float: the scalar `send()` overload does
  not compile against this min-api pin.
- **AEC-only:** `@postfilter` selects `tap::mu::aec_chain`, configured from
  `tap::mu::aec_chain_preset(block, partitions, sr)` — the preset MuTap's ITU battery certifies, with
  time constants scaled to the sample rate. Because of that scaling it has a `dspsetup` message that
  republishes when the rate changes; the bare engines are rate-agnostic.

## Adding an external

Copy an existing project folder — nothing else registers it (root CMake `SUBDIRLIST`s
`source/projects/`), but everything is name-driven:

1. `source/projects/<name>_tilde/` with `<name>_tilde.cpp` + `CMakeLists.txt`. `~` in the Max object
   name becomes `_tilde` in the folder/target name; the Min build renames the bundle back.
2. `<name>_tilde_test.cpp` beside it gets a Catch2 executable automatically (the project CMakeLists
   includes `min-object-unittest.cmake` when the file exists). That test target does **not** inherit
   the external's libraries — it must `target_link_libraries(... mutap)` again. The test source
   `#include`s the object's `.cpp` directly.
3. `help/<object>.maxhelp` and `docs/<object>.maxref.xml`, one each — CI counts built externals
   against the number of project directories, so a new folder that fails to build fails the job.

Tests here are **Min-level only** (attribute defaults, block rounding, `mu` clamping) against
min-api's mock kernel, and run on the macOS job only — the mock kernel is Apple/Windows-specific.
DSP correctness is MuTap's CI's job (`tests/test_aec.cpp` there pins the measured double-talk
behavior).

## CMake facts worth knowing before editing the root file

Each oddity in `CMakeLists.txt` is load-bearing and commented; the short version:

- **C++20 is re-forced per target after `add_subdirectory`** because Min's `min-posttarget.cmake`
  pins every external to C++17 and the test harness pins every `_test` to 17. On Apple the harness
  also defines `CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS`, which breaks under C++20 — the root file
  cancels it for `*_test` targets. MuTap requires C++20; don't reintroduce per-object overrides.
- **macOS**: universal by default (an explicit `-DCMAKE_OSX_ARCHITECTURES` still wins; CI fails via
  `lipo` if a build comes out thin) and `-mmacosx-version-min=11.0` is appended as a global
  compile/link option because Min force-pins the deployment target to 10.11 per object.
- **Windows**: `NOMINMAX`/`WIN32_LEAN_AND_MEAN`, static CRT (to match min-devkit's `/MT` against
  MuTap's CMP0091-NEW FFT target), `/bigobj`, and `/FIalgorithm` **for C++ only** — force-including
  it into the Ooura FFT's C sources trips STL1003.
- **`package-info.json` is generated** from `package-info.json.in` at configure time and is
  gitignored. Edit the template. (The `set(PACKAGE_VERSION "0.0.1")` above the include predates the
  template and now only backstops the Windows verinfo step.)
- A configure that dies with *"the current CMakeCache.txt directory ... is different"* means a stale
  `build/` from before the repos moved under `~/code/tap`; `build*/` is gitignored — delete it.
