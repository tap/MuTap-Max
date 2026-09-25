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
      40.0
     ],
     "text": "Acoustic feedback (howling) canceller. Left inlet: the microphone. Right inlet: the SAME signal the patch sends to the speaker (the reference). Left outlet: the cleaned mic, delayed by @block samples. Middle outlet: the IPC double-talk indicator (0..1). Right outlet: the convergence statistics, uncertainty_db shadow_ratio_db."
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
     "numoutlets": 3,
     "patching_rect": [
      40.0,
      200.0,
      160.0,
      22.0
     ],
     "text": "mutap.afc~ 2048",
     "outlettype": [
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
     "text": "IPC 0..1 (middle outlet): high = feedback dominates (adapting hard), low = you are talking (double-talk, updates gated). Meter it to watch the M4 robustness layer work."
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
     "text": "Example convergence policy (patch-side: mutap.afc~ applies no thresholds)",
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
      1
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
      2
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
  ]
 }
}