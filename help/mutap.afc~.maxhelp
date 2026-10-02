{
 "patcher": {
  "fileversion": 1,
  "appversion": {
   "major": 8,
   "minor": 0,
   "revision": 0,
   "architecture": "x64",
   "modernui": 1
  },
  "classnamespace": "box",
  "rect": [
   60.0,
   80.0,
   980.0,
   820.0
  ],
  "bglocked": 0,
  "openinpresentation": 0,
  "default_fontsize": 12.0,
  "default_fontface": 0,
  "default_fontname": "Arial",
  "gridonopen": 1,
  "gridsize": [
   15.0,
   15.0
  ],
  "gridsnaponopen": 1,
  "objectsnaponopen": 1,
  "statusbarvisible": 2,
  "toolbarvisible": 1,
  "boxes": [
   {
    "box": {
     "id": "obj-1",
     "maxclass": "newobj",
     "numinlets": 0,
     "numoutlets": 0,
     "patching_rect": [
      20.0,
      20.0,
      50.0,
      22.0
     ],
     "text": "p basic",
     "patcher": {
      "fileversion": 1,
      "appversion": {
       "major": 8,
       "minor": 0,
       "revision": 0,
       "architecture": "x64",
       "modernui": 1
      },
      "classnamespace": "box",
      "rect": [
       0.0,
       26.0,
       980.0,
       820.0
      ],
      "bglocked": 0,
      "openinpresentation": 0,
      "default_fontsize": 12.0,
      "default_fontface": 0,
      "default_fontname": "Arial",
      "gridonopen": 1,
      "gridsize": [
       15.0,
       15.0
      ],
      "gridsnaponopen": 1,
      "objectsnaponopen": 1,
      "statusbarvisible": 2,
      "toolbarvisible": 1,
      "boxes": [
       {
        "box": {
         "id": "obj-1",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          20.0,
          14.0,
          400.0,
          22.0
         ],
         "text": "mutap.afc~",
         "fontsize": 16.0
        }
       },
       {
        "box": {
         "id": "obj-2",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          20.0,
          38.0,
          700.0,
          54.0
         ],
         "text": "Acoustic feedback (howling) canceller. Left inlet: the microphone. Right inlet: the SAME signal the patch sends to the speaker (the reference). Outlets, left to right: the cleaned mic, delayed by @block samples; the guard's post-reverb gain (a signal, 1 while @guard is off; see the guard tab); the IPC double-talk indicator (0..1); the convergence statistics, uncertainty_db shadow_ratio_db."
        }
       },
       {
        "box": {
         "id": "obj-3",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          40.0,
          120.0,
          50.0,
          22.0
         ],
         "text": "adc~",
         "outlettype": [
          "signal",
          "signal"
         ]
        }
       },
       {
        "box": {
         "id": "obj-4",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 4,
         "patching_rect": [
          40.0,
          200.0,
          160.0,
          22.0
         ],
         "text": "mutap.afc~ 2048",
         "outlettype": [
          "signal",
          "signal",
          "",
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-5",
         "maxclass": "gain~",
         "numinlets": 2,
         "numoutlets": 2,
         "patching_rect": [
          40.0,
          260.0,
          30.0,
          120.0
         ],
         "outlettype": [
          "signal",
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-6",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 0,
         "patching_rect": [
          40.0,
          420.0,
          50.0,
          22.0
         ],
         "text": "dac~",
         "outlettype": []
        }
       },
       {
        "box": {
         "id": "obj-7",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          90.0,
          262.0,
          560.0,
          40.0
         ],
         "text": "The closed loop: mic -> afc~ -> gain -> speaker. Raise the gain slider toward howling onset; the canceller buys added stable gain. The gain~ output is tapped back into afc~'s right inlet \u2014 the reference MUST be the signal that actually reaches the speaker."
        }
       },
       {
        "box": {
         "id": "obj-8",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          180.0,
          240.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-9",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          244.0,
          240.0,
          420.0,
          22.0
         ],
         "text": "IPC 0..1 (third outlet): high = feedback dominates (adapting hard), low = you are talking (double-talk, updates gated). Meter it to watch the M4 robustness layer work."
        }
       },
       {
        "box": {
         "id": "obj-10",
         "maxclass": "toggle",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          480.0,
          96.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-11",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          510.0,
          120.0,
          70.0,
          22.0
         ],
         "text": "adapt $1",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-12",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          586.0,
          120.0,
          260.0,
          20.0
         ],
         "text": "freeze / resume adaptation (default on)"
        }
       },
       {
        "box": {
         "id": "obj-13",
         "maxclass": "toggle",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          480.0,
          150.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-14",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          510.0,
          174.0,
          64.0,
          22.0
         ],
         "text": "gate $1",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-15",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          580.0,
          174.0,
          340.0,
          20.0
         ],
         "text": "IPC step scaling + transient freeze (default on; rebuilds)"
        }
       },
       {
        "box": {
         "id": "obj-16",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          480.0,
          204.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-17",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          546.0,
          204.0,
          60.0,
          22.0
         ],
         "text": "mu $1",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-18",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          612.0,
          204.0,
          300.0,
          20.0
         ],
         "text": "NLMS step size, (0, 2), default 0.5 (applied live)"
        }
       },
       {
        "box": {
         "id": "obj-19",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          480.0,
          360.0,
          44.0,
          22.0
         ],
         "text": "reset",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-20",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          530.0,
          360.0,
          300.0,
          20.0
         ],
         "text": "zero the learned feedback-path estimate"
        }
       },
       {
        "box": {
         "id": "obj-21",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          480.0,
          330.0,
          80.0,
          22.0
         ],
         "text": "block 256",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-22",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          566.0,
          330.0,
          380.0,
          20.0
         ],
         "text": "block size = adaptation hop = added latency (power of 2; rebuilds)"
        }
       },
       {
        "box": {
         "id": "obj-23",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          20.0,
          460.0,
          700.0,
          54.0
         ],
         "text": "Why PEM: in a closed loop the speaker signal is correlated with your voice, so a naive adaptive filter cancels program material, not feedback. mutap.afc~ re-fits a near-end model (LPC + pitch) every block, prewhitens both signals with it, and adapts on the whitened pair while cancelling on the raw ones (FDAF-PEM-AFROW). Creation arg = filter length in samples (default 2048); partitions = filter length / block."
        }
       },
       {
        "box": {
         "id": "obj-24",
         "maxclass": "toggle",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          480.0,
          234.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "int"
         ],
         "parameter_enable": 0
        }
       },
       {
        "box": {
         "id": "obj-25",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          510.0,
          258.0,
          64.0,
          22.0
         ],
         "outlettype": [
          ""
         ],
         "text": "warp $1"
        }
       },
       {
        "box": {
         "id": "obj-26",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          580.0,
          258.0,
          400.0,
          20.0
         ],
         "outlettype": [],
         "text": "frequency-warped near-end model for music/tonal sources (default off; rebuilds, keeps IPC scaling on)"
        }
       },
       {
        "box": {
         "id": "obj-27",
         "maxclass": "toggle",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          480.0,
          264.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "int"
         ],
         "parameter_enable": 0
        }
       },
       {
        "box": {
         "id": "obj-28",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          510.0,
          288.0,
          80.0,
          22.0
         ],
         "outlettype": [
          ""
         ],
         "text": "kalman $1"
        }
       },
       {
        "box": {
         "id": "obj-29",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          596.0,
          288.0,
          400.0,
          20.0
         ],
         "outlettype": [],
         "text": "v2 Kalman engine: mu ignored, gate = burst floor (default off; rebuilds)"
        }
       },
       {
        "box": {
         "id": "obj-30",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          180.0,
          314.0,
          150.0,
          22.0
         ],
         "text": "unpack 0. 0.",
         "outlettype": [
          "float",
          "float"
         ]
        }
       },
       {
        "box": {
         "id": "obj-31",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          180.0,
          344.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-32",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          310.0,
          344.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-33",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          100.0,
          370.0,
          150.0,
          34.0
         ],
         "text": "uncertainty dB (Kalman core; 0 dB with NLMS)"
        }
       },
       {
        "box": {
         "id": "obj-34",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          260.0,
          370.0,
          210.0,
          34.0
         ],
         "text": "shadow ratio dB (0 dB with @shadow 0)"
        }
       },
       {
        "box": {
         "id": "obj-35",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          100.0,
          404.0,
          370.0,
          50.0
         ],
         "text": "Right outlet, every 8 blocks: raw statistics, no thresholds. Uncertainty: sum P / sum P at reset (0 dB = nothing identified; falls as the path is learned). Shadow ratio: main / shadow residual power; rises toward 0 dB and past it when the path moves."
        }
       },
       {
        "box": {
         "id": "obj-36",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          480.0,
          392.0,
          64.0,
          22.0
         ],
         "text": "shadow 2",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-37",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          550.0,
          392.0,
          64.0,
          22.0
         ],
         "text": "shadow 0",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-38",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          620.0,
          392.0,
          350.0,
          48.0
         ],
         "text": "shadow comparator partitions (default 2, 0 = off; clamped to the partition count at each build; rebuilds). Measured with the Kalman engine only."
        }
       },
       {
        "box": {
         "id": "obj-39",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          20.0,
          530.0,
          700.0,
          22.0
         ],
         "text": "Example convergence policy (patch-side: without @guard, mutap.afc~ applies no thresholds; the guard tab shows the built-in safety layer)",
         "fontface": 1
        }
       },
       {
        "box": {
         "id": "obj-40",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          40.0,
          560.0,
          230.0,
          22.0
         ],
         "text": "expr ($f1 < -23.8) && ($f2 < -1.24)",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-41",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 3,
         "patching_rect": [
          40.0,
          590.0,
          50.0,
          22.0
         ],
         "text": "change",
         "outlettype": [
          "",
          "int",
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-42",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          40.0,
          620.0,
          50.0,
          22.0
         ],
         "text": "t b i",
         "outlettype": [
          "bang",
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-43",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          40.0,
          650.0,
          70.0,
          22.0
         ],
         "text": "delay 300",
         "outlettype": [
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-44",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          40.0,
          680.0,
          40.0,
          22.0
         ],
         "text": "i",
         "outlettype": [
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-45",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 3,
         "patching_rect": [
          40.0,
          710.0,
          50.0,
          22.0
         ],
         "text": "change",
         "outlettype": [
          "",
          "int",
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-46",
         "maxclass": "toggle",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          40.0,
          740.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-47",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          70.0,
          742.0,
          220.0,
          20.0
         ],
         "text": "converged (each state held 0.3 s)"
        }
       },
       {
        "box": {
         "id": "obj-48",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          300.0,
          560.0,
          660.0,
          180.0
         ],
         "text": "\"Converged\" when uncertainty < -23.8 dB AND shadow ratio < -1.24 dB; a state flips only after the condition has held 0.3 s on the other side (delay 300 restarts on every change). Needs @kalman 1: with the NLMS core the uncertainty reads 0 dB and this never says converged.\n\nThese thresholds were calibrated at @block 64 / 48 kHz on 1024- and 2048-tap filters with @kalman 1 and @shadow 2 (medians over 50 cold starts in MuTap's convergence-indicator experiment). They are a calibration for that loop, not constants: recalibrate for any other block size, rate, filter length or room.\n\nMeasured there: it detects a walk to a different room at the change, but reports reconvergence 0.13-0.26 s early (ahead of the filter's measured misalignment). The uncertainty statistic regrows in digital silence, so a silent input drifts back to not converged."
        }
       }
      ],
      "lines": [
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-3",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-5",
          0
         ],
         "source": [
          "obj-4",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-6",
          0
         ],
         "source": [
          "obj-5",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-6",
          1
         ],
         "source": [
          "obj-5",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          1
         ],
         "source": [
          "obj-5",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-8",
          0
         ],
         "source": [
          "obj-4",
          2
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-11",
          0
         ],
         "source": [
          "obj-10",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-11",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-14",
          0
         ],
         "source": [
          "obj-13",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-14",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-17",
          0
         ],
         "source": [
          "obj-16",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-17",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-19",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-21",
          0
         ]
        }
       },
       {
        "patchline": {
         "source": [
          "obj-24",
          0
         ],
         "destination": [
          "obj-25",
          0
         ]
        }
       },
       {
        "patchline": {
         "source": [
          "obj-25",
          0
         ],
         "destination": [
          "obj-4",
          0
         ]
        }
       },
       {
        "patchline": {
         "source": [
          "obj-27",
          0
         ],
         "destination": [
          "obj-28",
          0
         ]
        }
       },
       {
        "patchline": {
         "source": [
          "obj-28",
          0
         ],
         "destination": [
          "obj-4",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-30",
          0
         ],
         "source": [
          "obj-4",
          3
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-31",
          0
         ],
         "source": [
          "obj-30",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-32",
          0
         ],
         "source": [
          "obj-30",
          1
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-36",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-37",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-40",
          0
         ],
         "source": [
          "obj-30",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-40",
          1
         ],
         "source": [
          "obj-30",
          1
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-41",
          0
         ],
         "source": [
          "obj-40",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-42",
          0
         ],
         "source": [
          "obj-41",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-44",
          1
         ],
         "source": [
          "obj-42",
          1
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-43",
          0
         ],
         "source": [
          "obj-42",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-44",
          0
         ],
         "source": [
          "obj-43",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-45",
          0
         ],
         "source": [
          "obj-44",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-46",
          0
         ],
         "source": [
          "obj-45",
          0
         ]
        }
       }
      ],
      "showontab": 1
     }
    }
   },
   {
    "box": {
     "id": "obj-2",
     "maxclass": "newobj",
     "numinlets": 0,
     "numoutlets": 0,
     "patching_rect": [
      20.0,
      50.0,
      50.0,
      22.0
     ],
     "text": "p guard",
     "patcher": {
      "fileversion": 1,
      "appversion": {
       "major": 8,
       "minor": 0,
       "revision": 0,
       "architecture": "x64",
       "modernui": 1
      },
      "classnamespace": "box",
      "rect": [
       0.0,
       26.0,
       980.0,
       820.0
      ],
      "bglocked": 0,
      "openinpresentation": 0,
      "default_fontsize": 12.0,
      "default_fontface": 0,
      "default_fontname": "Arial",
      "gridonopen": 1,
      "gridsize": [
       15.0,
       15.0
      ],
      "gridsnaponopen": 1,
      "objectsnaponopen": 1,
      "statusbarvisible": 2,
      "toolbarvisible": 1,
      "showontab": 1,
      "boxes": [
       {
        "box": {
         "id": "obj-1",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          20.0,
          14.0,
          600.0,
          22.0
         ],
         "text": "mutap.afc~ - the safety layer (@guard)",
         "fontsize": 16.0
        }
       },
       {
        "box": {
         "id": "obj-2",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          20.0,
          38.0,
          930.0,
          54.0
         ],
         "text": "@guard 1 runs MuTap's howl_guard on this mic: the cleaned output sits @arming dB down (ARMING) until the canceller's verdict (shadow_ratio_db < @d_db AND uncertainty_db < @a_db) has held @hold s with the howl detector quiet, then opens; a howl trip ducks it @duck dB. It needs @kalman 1 and @shadow > 0. One mutap.afc~ is one mic: two objects duck independently (MuTap's afc_chain attributes a shared howl to a mic; this object cannot)."
        }
       },
       {
        "box": {
         "id": "obj-3",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          40.0,
          110.0,
          50.0,
          22.0
         ],
         "text": "adc~",
         "outlettype": [
          "signal",
          "signal"
         ]
        }
       },
       {
        "box": {
         "id": "obj-4",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 4,
         "patching_rect": [
          40.0,
          200.0,
          220.0,
          22.0
         ],
         "text": "mutap.afc~ 2048 @kalman 1 @guard 1",
         "outlettype": [
          "signal",
          "signal",
          "",
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-5",
         "maxclass": "newobj",
         "numinlets": 5,
         "numoutlets": 1,
         "patching_rect": [
          110.0,
          250.0,
          150.0,
          22.0
         ],
         "text": "comb~ 100 37.1 0. 1. 0.75",
         "outlettype": [
          "signal"
         ]
        }
       },
       {
        "box": {
         "id": "obj-6",
         "maxclass": "newobj",
         "numinlets": 5,
         "numoutlets": 1,
         "patching_rect": [
          110.0,
          280.0,
          150.0,
          22.0
         ],
         "text": "comb~ 100 41.3 0. 1. 0.72",
         "outlettype": [
          "signal"
         ]
        }
       },
       {
        "box": {
         "id": "obj-7",
         "maxclass": "newobj",
         "numinlets": 3,
         "numoutlets": 1,
         "patching_rect": [
          110.0,
          310.0,
          120.0,
          22.0
         ],
         "text": "allpass~ 50 5. 0.7",
         "outlettype": [
          "signal"
         ]
        }
       },
       {
        "box": {
         "id": "obj-8",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          110.0,
          340.0,
          40.0,
          22.0
         ],
         "text": "*~",
         "outlettype": [
          "signal"
         ]
        }
       },
       {
        "box": {
         "id": "obj-9",
         "maxclass": "gain~",
         "numinlets": 2,
         "numoutlets": 2,
         "patching_rect": [
          40.0,
          380.0,
          30.0,
          120.0
         ],
         "outlettype": [
          "signal",
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-10",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 0,
         "patching_rect": [
          40.0,
          520.0,
          50.0,
          22.0
         ],
         "text": "dac~",
         "outlettype": []
        }
       },
       {
        "box": {
         "id": "obj-11",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          270.0,
          250.0,
          300.0,
          84.0
         ],
         "text": "A stand-in reverb (two comb~ into allpass~): put yours here. The guard's gain is already on the cleaned output, so a duck cuts what enters the reverb; the second outlet repeats the duck after it (*~), so a charged tail is cut too - MuTap's bus stage."
        }
       },
       {
        "box": {
         "id": "obj-12",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          160.0,
          341.0,
          400.0,
          34.0
         ],
         "text": "Second outlet: 1 except while ducked or releasing, where it is the duck relative to the restore level (ARMING and strike levels are not applied twice)."
        }
       },
       {
        "box": {
         "id": "obj-13",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          600.0,
          380.0,
          50.0,
          22.0
         ],
         "text": "open",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-14",
         "maxclass": "toggle",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          660.0,
          380.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-15",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 2,
         "patching_rect": [
          600.0,
          420.0,
          70.0,
          22.0
         ],
         "text": "sfplay~",
         "outlettype": [
          "signal",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-16",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          690.0,
          410.0,
          280.0,
          96.0
         ],
         "text": "Backing track: summed into the speaker feed after the guard, and into the reference. While ARMING holds the voice down it is the canceller's excitation: in MuTap's simulated cold starts every run declared with it, none without it in 20 s (docs/howl-guard.md)."
        }
       },
       {
        "box": {
         "id": "obj-17",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          600.0,
          100.0,
          360.0,
          20.0
         ],
         "text": "1. Soundcheck (needs the guard running)",
         "fontface": 1
        }
       },
       {
        "box": {
         "id": "obj-18",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          600.0,
          124.0,
          64.0,
          22.0
         ],
         "text": "calibrate",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-19",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          670.0,
          124.0,
          80.0,
          22.0
         ],
         "text": "calibrate 0",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-20",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          600.0,
          148.0,
          360.0,
          62.0
         ],
         "text": "Play the song through the loop for @cal_s (30 s): the guard then runs on the medians of the two statistics plus @cal_margin_d / @cal_margin_a (4 / 3 dB) and sends calibrate_done d_db a_db. 'calibrate 0' ends it early."
        }
       },
       {
        "box": {
         "id": "obj-21",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          600.0,
          214.0,
          360.0,
          20.0
         ],
         "text": "2. The deployment cap",
         "fontface": 1
        }
       },
       {
        "box": {
         "id": "obj-22",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          600.0,
          238.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ],
         "maximum": 0.0
        }
       },
       {
        "box": {
         "id": "obj-23",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          666.0,
          238.0,
          60.0,
          22.0
         ],
         "text": "cap $1",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-24",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          732.0,
          238.0,
          64.0,
          22.0
         ],
         "text": "cap none",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-25",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          600.0,
          262.0,
          360.0,
          76.0
         ],
         "text": "dB re your operating gain, at most 0 (MuTap's protocol: the dry limit - 6 dB). After 10 s in ARMING without a verdict the guard says unprotected; with a cap it opens to it (open_capped), with none it stays in ARMING, @arming dB down."
        }
       },
       {
        "box": {
         "id": "obj-26",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          600.0,
          342.0,
          40.0,
          22.0
         ],
         "text": "clear",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-27",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          644.0,
          342.0,
          330.0,
          20.0
         ],
         "text": "strikes 0, restore level 0 dB, latch released"
        }
       },
       {
        "box": {
         "id": "obj-28",
         "maxclass": "toggle",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          820.0,
          100.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-29",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          850.0,
          100.0,
          60.0,
          22.0
         ],
         "text": "guard $1",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-30",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          850.0,
          124.0,
          44.0,
          22.0
         ],
         "text": "reset",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-31",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          900.0,
          124.0,
          70.0,
          34.0
         ],
         "text": "back to ARMING"
        }
       },
       {
        "box": {
         "id": "obj-32",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          300.0,
          560.0,
          400.0,
          20.0
         ],
         "text": "3. The guard, every 8 blocks (right outlet)",
         "fontface": 1
        }
       },
       {
        "box": {
         "id": "obj-33",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 2,
         "patching_rect": [
          300.0,
          584.0,
          140.0,
          22.0
         ],
         "text": "route calibrate_done",
         "outlettype": [
          "",
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-34",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          300.0,
          614.0,
          70.0,
          22.0
         ],
         "text": "unpack 0. 0.",
         "outlettype": [
          "float",
          "float"
         ]
        }
       },
       {
        "box": {
         "id": "obj-35",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          300.0,
          644.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-36",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          366.0,
          644.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-37",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          300.0,
          668.0,
          200.0,
          48.0
         ],
         "text": "calibrate_done d_db a_db: store them; set @d_db / @a_db to restore the soundcheck after a rebuild."
        }
       },
       {
        "box": {
         "id": "obj-38",
         "maxclass": "newobj",
         "numinlets": 1,
         "numoutlets": 6,
         "patching_rect": [
          520.0,
          614.0,
          200.0,
          22.0
         ],
         "text": "unpack 0. 0. s 0. 0 0",
         "outlettype": [
          "float",
          "float",
          "",
          "float",
          "int",
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-39",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          520.0,
          680.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-40",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          586.0,
          680.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-41",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          652.0,
          644.0,
          70.0,
          22.0
         ],
         "text": "prepend set",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-42",
         "maxclass": "message",
         "numinlets": 2,
         "numoutlets": 1,
         "patching_rect": [
          652.0,
          680.0,
          90.0,
          22.0
         ],
         "text": "",
         "outlettype": [
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-43",
         "maxclass": "flonum",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          748.0,
          680.0,
          60.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-44",
         "maxclass": "toggle",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          814.0,
          680.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "int"
         ]
        }
       },
       {
        "box": {
         "id": "obj-45",
         "maxclass": "number",
         "numinlets": 1,
         "numoutlets": 2,
         "patching_rect": [
          844.0,
          680.0,
          40.0,
          22.0
         ],
         "outlettype": [
          "",
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-46",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          520.0,
          706.0,
          460.0,
          20.0
         ],
         "text": "uncertainty_db  shadow_ratio_db  guard_state  gain_db  unprotected  strikes"
        }
       },
       {
        "box": {
         "id": "obj-47",
         "maxclass": "newobj",
         "numinlets": 2,
         "numoutlets": 3,
         "patching_rect": [
          652.0,
          740.0,
          150.0,
          22.0
         ],
         "text": "sel ducked latched",
         "outlettype": [
          "bang",
          "bang",
          ""
         ]
        }
       },
       {
        "box": {
         "id": "obj-48",
         "maxclass": "button",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          652.0,
          770.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-49",
         "maxclass": "button",
         "numinlets": 1,
         "numoutlets": 1,
         "patching_rect": [
          700.0,
          770.0,
          24.0,
          24.0
         ],
         "outlettype": [
          "bang"
         ]
        }
       },
       {
        "box": {
         "id": "obj-50",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          730.0,
          770.0,
          250.0,
          34.0
         ],
         "text": "route on the state symbol: ducked, latched"
        }
       },
       {
        "box": {
         "id": "obj-51",
         "maxclass": "comment",
         "numinlets": 1,
         "numoutlets": 0,
         "patching_rect": [
          20.0,
          560.0,
          270.0,
          230.0
         ],
         "text": "guard_state: arming (held @arming dB down until the verdict declares), open (0 dB, less 3 dB per strike), open_capped (no verdict yet: at the cap, unprotected), ducked (@duck dB under the restore level, on a howl trip or a lost verdict), releasing (ramping back up), latched (three strikes: at the floor, unprotected). gain_db is the gain at the end of the last block; a duck releases on the verdict or after @rearm s x 2^strikes. reset returns to arming; so does every rebuild."
        }
       }
      ],
      "lines": [
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-3",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-9",
          0
         ],
         "source": [
          "obj-4",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-5",
          0
         ],
         "source": [
          "obj-4",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-6",
          0
         ],
         "source": [
          "obj-4",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-7",
          0
         ],
         "source": [
          "obj-5",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-7",
          0
         ],
         "source": [
          "obj-6",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-8",
          0
         ],
         "source": [
          "obj-7",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-8",
          1
         ],
         "source": [
          "obj-4",
          1
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-9",
          0
         ],
         "source": [
          "obj-8",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-10",
          0
         ],
         "source": [
          "obj-9",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-10",
          1
         ],
         "source": [
          "obj-9",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          1
         ],
         "source": [
          "obj-9",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-15",
          0
         ],
         "source": [
          "obj-13",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-15",
          0
         ],
         "source": [
          "obj-14",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-10",
          0
         ],
         "source": [
          "obj-15",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-10",
          1
         ],
         "source": [
          "obj-15",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          1
         ],
         "source": [
          "obj-15",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-18",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-19",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-23",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-24",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-26",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-29",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-4",
          0
         ],
         "source": [
          "obj-30",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-23",
          0
         ],
         "source": [
          "obj-22",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-29",
          0
         ],
         "source": [
          "obj-28",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-33",
          0
         ],
         "source": [
          "obj-4",
          3
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-34",
          0
         ],
         "source": [
          "obj-33",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-35",
          0
         ],
         "source": [
          "obj-34",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-36",
          0
         ],
         "source": [
          "obj-34",
          1
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-38",
          0
         ],
         "source": [
          "obj-33",
          1
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-39",
          0
         ],
         "source": [
          "obj-38",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-40",
          0
         ],
         "source": [
          "obj-38",
          1
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-41",
          0
         ],
         "source": [
          "obj-38",
          2
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-42",
          0
         ],
         "source": [
          "obj-41",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-43",
          0
         ],
         "source": [
          "obj-38",
          3
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-44",
          0
         ],
         "source": [
          "obj-38",
          4
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-45",
          0
         ],
         "source": [
          "obj-38",
          5
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-47",
          0
         ],
         "source": [
          "obj-38",
          2
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-48",
          0
         ],
         "source": [
          "obj-47",
          0
         ]
        }
       },
       {
        "patchline": {
         "destination": [
          "obj-49",
          0
         ],
         "source": [
          "obj-47",
          1
         ]
        }
       }
      ]
     }
    }
   }
  ],
  "lines": []
 }
}