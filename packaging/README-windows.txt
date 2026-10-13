CTR Native Expanded Windows x86 {{VERSION}} build

Game assets are not included.

Normal setup:
- Create an assets folder next to ctr_native_expanded.exe and put your own
  NTSC-U retail CTR disc image there:

  assets/
    ctr-u.bin

- Run:

  ctr_native_expanded.exe

The disc image must be the common single-track raw PSX BIN layout:
MODE2/2352 sectors, with the data track starting at byte 0.
A cooked 2048-byte ISO does not preserve the XA/STR sector data needed for
audio and video playback.

Extracted asset override:

Extracted files are optional and mostly useful for development, modding, and
debugging. If present, they override files from ctr-u.bin.

Windows requirements:
- Windows x86 or x86_64
- OpenGL 3.3 capable GPU/driver

No MSYS2 install is required to run this release build.

This build also ships config.ini and controls.ini, copied from the project's
default settings so the options are visible and editable from the first run.
Nothing needs to be extracted for them; the game reads the files next to it.
