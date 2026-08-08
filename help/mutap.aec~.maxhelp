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
   1020.0,
   700.0
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
     "text": "mutap.aec~",
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
      720.0,
      40.0
     ],
     "text": "Acoustic echo canceller. Left inlet: the microphone (echo + you). Right inlet: the far-end reference \u2014 the SAME signal the patch sends to the speaker. Left outlet: the mic with the echo removed, delayed by @block samples. Right outlet: the IPC double-talk indicator (0..1)."
    }
   },
   {
    "box": {
     "id": "obj-3",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      40.0,
      100.0,
      52.0,
      22.0
     ],
     "text": "noise~",
     "outlettype": [
      "signal"
     ]
    }
   },
   {
    "box": {
     "id": "obj-4",
     "maxclass": "gain~",
     "numinlets": 2,
     "numoutlets": 2,
     "patching_rect": [
      40.0,
      130.0,
      30.0,
      100.0
     ],
     "outlettype": [
      "signal",
      "int"
     ]
    }
   },
   {
    "box": {
     "id": "obj-5",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      90.0,
      240.0,
      70.0,
      22.0
     ],
     "text": "tapin~ 100",
     "outlettype": [
      "tapconnect"
     ]
    }
   },
   {
    "box": {
     "id": "obj-6",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      90.0,
      270.0,
      72.0,
      22.0
     ],
     "text": "tapout~ 42",
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
      90.0,
      300.0,
      100.0,
      22.0
     ],
     "text": "lores~ 2500 0.3",
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
      90.0,
      330.0,
      52.0,
      22.0
     ],
     "text": "*~ 0.4",
     "outlettype": [
      "signal"
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
      200.0,
      270.0,
      520.0,
      54.0
     ],
     "text": "The simulated room: 42 ms delay + lowpass + attenuation stands in for the speaker-to-mic echo path, so this patcher demonstrates without an acoustic loop. In a real rig, delete this chain \u2014 the echo arrives through the air, and the mic signal goes straight to the left inlet. The reference MUST be tapped where the signal actually reaches the speaker (post-fader)."
    }
   },
   {
    "box": {
     "id": "obj-10",
     "maxclass": "newobj",
     "numinlets": 1,
     "numoutlets": 2,
     "patching_rect": [
      240.0,
      100.0,
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
     "id": "obj-11",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      90.0,
      370.0,
      40.0,
      22.0
     ],
     "text": "+~",
     "outlettype": [
      "signal"
     ]
    }
   },
   {
    "box": {
     "id": "obj-12",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 2,
     "patching_rect": [
      90.0,
      410.0,
      160.0,
      22.0
     ],
     "text": "mutap.aec~ 2048",
     "outlettype": [
      "signal",
      ""
     ]
    }
   },
   {
    "box": {
     "id": "obj-13",
     "maxclass": "gain~",
     "numinlets": 2,
     "numoutlets": 2,
     "patching_rect": [
      90.0,
      450.0,
      30.0,
      100.0
     ],
     "outlettype": [
      "signal",
      "int"
     ]
    }
   },
   {
    "box": {
     "id": "obj-14",
     "maxclass": "newobj",
     "numinlets": 2,
     "numoutlets": 0,
     "patching_rect": [
      90.0,
      570.0,
      50.0,
      22.0
     ],
     "text": "dac~",
     "outlettype": []
    }
   },
   {
    "box": {
     "id": "obj-15",
     "maxclass": "flonum",
     "numinlets": 1,
     "numoutlets": 2,
     "patching_rect": [
      270.0,
      450.0,
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
     "id": "obj-16",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      334.0,
      450.0,
      420.0,
      40.0
     ],
     "text": "IPC 0..1 (right outlet): high = unmodeled echo dominates (adapting hard), low = you are talking over it (double-talk). Reports 0 with @kalman (its gating is internal)."
    }
   },
   {
    "box": {
     "id": "obj-17",
     "maxclass": "toggle",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      520.0,
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
     "id": "obj-18",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      550.0,
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
     "id": "obj-19",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      626.0,
      120.0,
      280.0,
      20.0
     ],
     "text": "freeze / resume adaptation (default on)"
    }
   },
   {
    "box": {
     "id": "obj-20",
     "maxclass": "toggle",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      520.0,
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
     "id": "obj-21",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      550.0,
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
     "id": "obj-22",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      620.0,
      174.0,
      380.0,
      20.0
     ],
     "text": "classic engine: IPC step scaling + transient freeze (default on; rebuilds)"
    }
   },
   {
    "box": {
     "id": "obj-23",
     "maxclass": "flonum",
     "numinlets": 1,
     "numoutlets": 2,
     "patching_rect": [
      520.0,
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
     "id": "obj-24",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      586.0,
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
     "id": "obj-25",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      652.0,
      204.0,
      300.0,
      20.0
     ],
     "text": "NLMS step size, (0, 2), default 0.5 (applied live)"
    }
   },
   {
    "box": {
     "id": "obj-26",
     "maxclass": "toggle",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      520.0,
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
     "id": "obj-27",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      550.0,
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
     "id": "obj-28",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      620.0,
      258.0,
      400.0,
      20.0
     ],
     "outlettype": [],
     "text": "frequency-warped near-end model for music/tonal material in the room (default off; rebuilds)"
    }
   },
   {
    "box": {
     "id": "obj-29",
     "maxclass": "toggle",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      520.0,
      288.0,
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
     "id": "obj-30",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      550.0,
      312.0,
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
     "id": "obj-31",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      636.0,
      312.0,
      400.0,
      20.0
     ],
     "outlettype": [],
     "text": "v2 Kalman engine \u2014 the measured double-talk winner (default off; rebuilds)"
    }
   },
   {
    "box": {
     "id": "obj-32",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      520.0,
      342.0,
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
     "id": "obj-33",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      606.0,
      342.0,
      380.0,
      20.0
     ],
     "text": "block size = adaptation hop = added latency (power of 2; rebuilds)"
    }
   },
   {
    "box": {
     "id": "obj-34",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      520.0,
      372.0,
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
     "id": "obj-35",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      570.0,
      372.0,
      300.0,
      20.0
     ],
     "text": "zero the learned echo-path estimate"
    }
   },
   {
    "box": {
     "id": "obj-36",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      20.0,
      600.0,
      760.0,
      95.0
     ],
     "text": "Try it: start the audio, raise the far-end fader \u2014 the noise 'echo' appears in the mic and the canceller learns it away within a second or two. Now TALK: that is double-talk, the hard part of echo cancellation. A naive filter would chase your voice and wreck its echo estimate; this one keeps adapting through it \u2014 the PEM near-end model whitens your voice out of the update, and @kalman 1 tracks it per frequency bin (the measured best, no detector, no tuning; see MuTap tests/test_aec.cpp). For the last 20-30 dB, @postfilter 1 engages the full ITU-certified chain: the residual the linear filter cannot reach is suppressed by coherence and replaced with comfort noise matched to the room, so the far end hears neither echo nor a breathing noise floor. @postfilter 2 swaps that suppressor for a small trained network (stronger single-talk echo removal on speech at equal transparency; the classical engine stays the certified default) \u2014 @model loads your own trained weights, and @block follows the model's trained block size. Creation arg = echo-path length in samples (default 2048); partitions = filter length / block."
    }
   },
   {
    "box": {
     "id": "obj-37",
     "maxclass": "toggle",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      520.0,
      402.0,
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
     "id": "obj-38",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      550.0,
      426.0,
      96.0,
      22.0
     ],
     "outlettype": [
      ""
     ],
     "text": "postfilter $1"
    }
   },
   {
    "box": {
     "id": "obj-39",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      656.0,
      426.0,
      380.0,
      33.0
     ],
     "text": "post-filter engine: 0 off, 1 the ITU-certified chain, 2 the learned (neural) chain (rebuilds; +1 block latency)"
    }
   },
   {
    "box": {
     "id": "obj-40",
     "maxclass": "toggle",
     "numinlets": 1,
     "numoutlets": 1,
     "patching_rect": [
      520.0,
      500.0,
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
     "id": "obj-41",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "patching_rect": [
      550.0,
      524.0,
      80.0,
      22.0
     ],
     "outlettype": [
      ""
     ],
     "text": "comfort $1"
    }
   },
   {
    "box": {
     "id": "obj-42",
     "maxclass": "comment",
     "numinlets": 1,
     "numoutlets": 0,
     "patching_rect": [
      640.0,
      524.0,
      376.0,
      20.0
     ],
     "text": "comfort-noise fill at the room's noise floor (postfilter only; default on)"
    }
   },
   {
    "box": {
     "id": "obj-90",
     "maxclass": "message",
     "numinlets": 2,
     "numoutlets": 1,
     "outlettype": [
      ""
     ],
     "patching_rect": [
      550.0,
      450.0,
      82.0,
      22.0
     ],
     "text": "postfilter 2"
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
      "obj-12",
      1
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
      "obj-14",
      1
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
      "obj-11",
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
      "obj-11",
      1
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
      "obj-12",
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
      "obj-13",
      0
     ],
     "source": [
      "obj-12",
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
      "obj-15",
      0
     ],
     "source": [
      "obj-12",
      1
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-18",
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
      "obj-12",
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
      "obj-21",
      0
     ],
     "source": [
      "obj-20",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-12",
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
     "destination": [
      "obj-24",
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
      "obj-12",
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
      "obj-27",
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
      "obj-12",
      0
     ],
     "source": [
      "obj-27",
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
      "obj-29",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-12",
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
      "obj-12",
      0
     ],
     "source": [
      "obj-32",
      0
     ]
    }
   },
   {
    "patchline": {
     "destination": [
      "obj-12",
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
      "obj-38",
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
      "obj-12",
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
      "obj-12",
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
      "obj-12",
      0
     ],
     "source": [
      "obj-90",
      0
     ]
    }
   }
  ]
 }
}