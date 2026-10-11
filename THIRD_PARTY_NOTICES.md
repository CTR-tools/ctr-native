# Third-Party Notices

This project vendors third-party software and contains modified third-party
derivatives. Keep this file with source and binary distributions of CTR Native
Expanded.

## PsyCross / Psy-X

Source: <https://github.com/OpenDriver2/PsyCross>

PsyCross provided the starting point for parts of upstream CTR Native's
Psy-Q-compatible PS1 hardware abstraction layer, including compatible GPU, GTE,
SPU, CD, and controller library interfaces. CTR Native now owns those headers
and native platform implementations in `include/` and `platform/` while
preserving Psy-Q-shaped APIs.

CTR Native Expanded contains modified/project-owned PsyCross derivatives in these
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


License: MIT
