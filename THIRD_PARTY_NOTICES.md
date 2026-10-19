# Third-Party Notices

This project vendors third-party software and contains modified third-party
derivatives. Keep this file with source and binary distributions of CTR Native.

## PsyCross / Psy-X

Source: <https://github.com/OpenDriver2/PsyCross>

PsyCross provided the starting point for parts of CTR Native's
Psy-Q-compatible PS1 hardware abstraction layer, including compatible GPU, GTE,
SPU, CD, and controller library interfaces. CTR Native now owns those headers
and native platform implementations in `include/` and `platform/` while
preserving Psy-Q-shaped APIs.

CTR Native contains modified/project-owned PsyCross derivatives in these
component areas:

- `include/psx/`: Psy-Q-compatible facade headers
- `include/platform/`: native GPU/renderer facade types and support headers
- `platform/`: native PS1 facade implementations, GTE/GPU/render support,
  platform shell code, and generated GL loader sources

Individual source files may carry narrower provenance notes where the original
PsyCross source path is useful during maintenance.

License: MIT

Copyright (c) 2020 REDRIVER2 Project

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

## PSn00bSDK

Source: <https://github.com/Lameguy64/PSn00bSDK>

Path: `include/psn00bsdk`

CTR Native vendors a small PSn00bSDK header subset for PS1/Psy-Q-compatible
types, constants, and inline helpers used by the shared source. CTR Native does
not vendor or link `libpsn00b` into the native PC executable.

`CTR_GteMoveMACToTranslation()` in `include/ctr_gte_transfer.h` is adapted from
PSn00bSDK's `gte_mvlvtr` macro in `include/psn00bsdk/include/inline_c.h`,
with a native implementation using CTR Native's existing GTE interface.
The adapted header is distributed under MPL 2.0 and retains the upstream
copyright notices:

- (C) 2019 Lameguy64
- (C) 2021-2022 Soapy (tweaked by spicyjpeg)

This notice applies to PSn00bSDK core files only. `mkpsxiso` and `dumpsxiso`
are separate GPLv2-or-later tools and are not distributed as part of CTR Native.

License: Mozilla Public License 2.0

The vendored header files retain their original copyright and license notices.
A copy of the MPL 2.0 license can be obtained at:
<https://mozilla.org/MPL/2.0/>

## SDL3

Path: `externals/SDL`

SDL3 provides cross-platform host windowing, input, timing, and audio device
support for CTR Native.

Vendored version: 3.4.10 (`release-3.4.10`)

Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

## maspsx

Source: <https://github.com/mkst/maspsx>

Path: `externals/maspsx`

CTR Native vendors `maspsx` at commit
`42b862c988fe7a13fe4e7ac0ebec90ed6b9fb763` for its PSYQ ASPSX-compatible
assembly transformation.

License: MIT

The full license is retained in `externals/maspsx/LICENSE`.

## GNU GCC 2.8.1 PSX toolchain

Sources:

- <https://github.com/decompals/old-gcc>
- <https://ftp.gnu.org/gnu/gcc/gcc-2.8.1.tar.gz>

Path: `externals/old-gcc`

CTR Native vendors the Decompals build recipes at tag `0.17`, the corresponding
GCC 2.8.1 source archive, and the tested Linux i386 PSX compiler release.

License: GNU GPL v2

The license is retained in `externals/old-gcc/GCC-COPYING`; exact source,
release, commit, and binary hashes are recorded in
`externals/old-gcc/UPSTREAM.md`.

## GNU Binutils 2.40

Sources:

- <https://github.com/Lameguy64/PSn00bSDK/releases/tag/v0.24>
- <https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.xz>

Path: `externals/psx-binutils`

CTR Native retains the four MIPS ELF programs used by the matching pipeline
from the PSn00bSDK v0.24 Linux toolchain distribution, together with the
corresponding GNU Binutils 2.40 source archive.

The applicable GNU licenses are retained in `externals/psx-binutils/COPYING`
and `externals/psx-binutils/COPYING3`; exact archive and binary hashes are
recorded in `externals/psx-binutils/UPSTREAM.md`.
