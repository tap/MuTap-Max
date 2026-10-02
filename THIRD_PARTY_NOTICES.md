# Third-Party Notices

MuTap-Max's own code is licensed under the MIT License (see `LICENSE`), © 2026
MuTap contributors. The externals it builds (`externals/*.mxo`, `externals/*.mxe64`)
also compile in the third-party code listed below, all of it header-only code reached
through the submodules, except the first entry: Ooura's FFT is a license record for
externals built from this repository's earlier trees. Every notice is quoted **verbatim, byte for byte**, from the
file named above it, at the submodule pins this repository records (tabs and
comment markers included where the notice is a source comment). Redistributions of
the built externals should include this file.

This is a record of what the externals contain and the terms each part carries, not
legal advice.

---

## Ooura FFT: license record only (no code at the pinned DspTap)

- **What is compiled in: none of it, in DspTap's maintainer's judgement.** At the
  pinned DspTap (`2137d86`, through MuTap `449cf6c`) every transform the externals
  run, through `tap::dsp::basic_real_fft` in MuTap's frequency-domain cores, is
  DspTap's srdif engine,
  `submodules/MuTap/submodules/dsptap/include/tap/dsp/fft/srdif.h`: a split-radix
  decimation-in-frequency engine written from the published literature under a
  recorded clean-room procedure, MIT (tap/DspTap#42, tuned for Hexagon in #44). The
  externals hold the double profiles only, and double always runs srdif. DspTap's
  fixed-point real post-pass, the other part once transcribed from the package, was
  re-derived from the literature at tap/DspTap#39; the externals do not instantiate
  it either way.
- **History, for externals built from older trees.** Every MuTap-Max tree before
  this bump compiles DspTap's C++20 port of `rdft`
  (`include/tap/dsp/fft/split_radix.h`, through MuTap `edf160e` and earlier pins):
  a derivative work, not the ORIGINAL package, marked
  `SPDX-License-Identifier: LicenseRef-Ooura AND MIT`, whose redistribution relied on
  the notice's modification grant. Externals built from those trees carry it, and
  the notice below applies to them. DspTap's `NOTICE.md` ("Which trees carry what")
  and MuTap's `THIRD_PARTY_NOTICES.md` give the same history by tree.
- **Canonical statement:** DspTap's `NOTICE.md` (the judgement, the clean-room
  records behind it in `docs/fft-design.md`, and its limits; a maintainer
  judgement, not legal advice), carried forward by MuTap's `THIRD_PARTY_NOTICES.md`.
  DspTap keeps the license record permanently at
  `submodules/MuTap/submodules/dsptap/third_party/ooura/readme.txt` and
  `LICENSES/LicenseRef-Ooura.txt` (quoted below), so the historical trees' SPDX
  reference resolves to a fixed path.
- **Notice**, verbatim from `submodules/MuTap/submodules/dsptap/LICENSES/LicenseRef-Ooura.txt`:

```text
Copyright:
    Copyright(C) 1996-2001 Takuya OOURA
    email: ooura@mmm.t.u-tokyo.ac.jp
    download: http://momonga.t.u-tokyo.ac.jp/~ooura/fft.html
    You may use, copy, modify this code for any purpose and
    without fee. You may distribute this ORIGINAL package.
```

## readerwriterqueue

- **What is compiled in:** Cameron Desrochers's single-producer/single-consumer queue,
  `source/min-api/include/readerwriterqueue/` (a submodule of min-api). min-api's
  `c74_min_api.h` includes `readerwriterqueue/readerwriterqueue.h` and builds its
  `fifo<>` on `moodycamel::ReaderWriterQueue`; both externals use it through their
  `thread_action::fifo` IPC outlet.
- **Licence:** Simplified BSD (BSD-2-Clause), whose second condition requires binary
  redistributions to reproduce the notice. `atomicops.h` also embeds Jeff Preshing's
  semaphore under a separate zlib licence, quoted second.
- **Notice**, verbatim from `source/min-api/include/readerwriterqueue/LICENSE.md`:

```text
This license applies to all the code in this repository except that written by third
parties, namely the files in benchmarks/ext, which have their own licenses, and Jeff
Preshing's semaphore implementation (used in the blocking queue) which has a zlib
license (embedded in atomicops.h).

Simplified BSD License:

Copyright (c) 2013-2015, Cameron Desrochers  
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or other materials
provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

- **zlib notice**, verbatim from `source/min-api/include/readerwriterqueue/atomicops.h`, lines 362–379:

```text
	// LICENSE:
	// Copyright (c) 2015 Jeff Preshing
	//
	// This software is provided 'as-is', without any express or implied
	// warranty. In no event will the authors be held liable for any damages
	// arising from the use of this software.
	//
	// Permission is granted to anyone to use this software for any purpose,
	// including commercial applications, and to alter it and redistribute it
	// freely, subject to the following restrictions:
	//
	// 1. The origin of this software must not be misrepresented; you must not
	//    claim that you wrote the original software. If you use this software
	//    in a product, an acknowledgement in the product documentation would be
	//    appreciated but is not required.
	// 2. Altered source versions must be plainly marked as such, and must not be
	//    misrepresented as being the original software.
	// 3. This notice may not be removed or altered from any source distribution.
```

## Murmur3

- **What is compiled in:** `source/min-api/include/murmur/Murmur3.h`, the constexpr
  MurmurHash3 min-api uses for symbol hashing (`c74_min_api.h` includes it).
- **Licence:** MIT, as the file states. The file names its source (a gist by
  mattyclarkson) and says "Distributed under the MIT License"; it carries no copyright
  line or licence text of its own, so its header is the whole notice there is.
- **Notice**, verbatim from `source/min-api/include/murmur/Murmur3.h`, lines 1–8:

```text
/*	
	This code comes from https://gist.github.com/mattyclarkson/5318077
	It is an implementation of the Murmur3 hashing algorithm:
		https://en.wikipedia.org/wiki/MurmurHash
		https://code.google.com/p/smhasher/

	Distributed under the MIT License.
*/
```

## min-api

- **What is compiled in:** Cycling '74's Min-API headers, `source/min-api/include/`.
- **Licence:** MIT. Verbatim from `source/min-api/License.md`:

```text
The MIT License

Copyright 2018, The Min-API Authors. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## Max SDK (max-sdk-base)

- **What is compiled in:** the Max SDK C headers min-api builds on,
  `source/min-api/max-sdk-base/c74support/`.
- **Licence:** MIT. Verbatim from `source/min-api/max-sdk-base/LICENSE.md`:

```text
Copyright (c) 2021, Cycling '74.
All rights reserved.

The software to which this license pertains is the Max SDK that consists of the C language header files and source code examples contained within this archive.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## MuTap and DspTap

- **What is compiled in:** MuTap's headers (`submodules/MuTap/include/`) and DspTap's
  (`submodules/MuTap/submodules/dsptap/include/`), all MIT at these pins (the Ooura
  history is above). The externals include `pem_afc.h`, `fd_kalman.h`, `nn_chain.h`
  and `postfilter.h` and what those include; MuTap's `frequency_shifter.h`, whose
  Hilbert coefficients MuTap's `THIRD_PARTY_NOTICES.md` records as of unstated
  licence, is not among them and is compiled into nothing this package builds.
- **Licence:** MIT. Verbatim from `submodules/MuTap/LICENSE`:

```text
MIT License

Copyright (c) 2026 MuTap contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

  and from `submodules/MuTap/submodules/dsptap/LICENSE`:

```text
MIT License

Copyright (c) 2025-2026 Timothy Place and the DspTap contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Note: this license covers DspTap's own wrapper code. Vendored third-party code
under third_party/ retains its own license — see NOTICE.md.
```
