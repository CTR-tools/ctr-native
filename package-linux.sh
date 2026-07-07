#!/usr/bin/env bash
# Package the current Linux x86 build for release.

set -euo pipefail

VERSION="${1:-beta-7_1}"
BUILD_DIR="${BUILD_DIR:-build}"
DIST_DIR="${DIST_DIR:-dist}"
PACKAGE_NAME="ctr-native-${VERSION}-linux-x86"
PACKAGE_DIR="${DIST_DIR}/${PACKAGE_NAME}"
BINARY_PATH="${BUILD_DIR}/ctr_native"

if [[ ! -x "${BINARY_PATH}" ]]; then
    echo "ERROR: missing executable: ${BINARY_PATH}" >&2
    echo "Run ./build.sh first." >&2
    exit 1
fi

rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"

cp "${BINARY_PATH}" "${PACKAGE_DIR}/"
cp LICENSE "${PACKAGE_DIR}/"
cp THIRD_PARTY_NOTICES.md "${PACKAGE_DIR}/"

cat >"${PACKAGE_DIR}/README.txt" <<EOF
CTR Native Linux x86 ${VERSION} build

Game assets are not included.

Simple setup:
- Put your own NTSC-U retail CTR disc image at:

assets/
  ctr-u.bin

- Run:

./ctr_native

The disc image must be the common single-track raw PSX BIN layout:
MODE2/2352 sectors, with the data track starting at byte 0.
A cooked 2048-byte ISO does not preserve the XA/STR sector data needed for
audio and video playback.

Extracted asset override:

Extracted files are optional and mostly useful for development, modding, and
debugging. If present, they override files from ctr-u.bin.

assets/
  BIGFILE.BIG
  SOUNDS/KART.HWL
  TEST.STR
  XA/
    ENG.XNF
    ENG/EXTRA/S00.XA ... S05.XA
    ENG/GAME/S00.XA ... S20.XA
    MUSIC/S00.XA ... S01.XA

XA files must preserve CD-XA sector data. Use 2336-byte Mode2/Form2 sector data
or 2352-byte raw sectors. 2048-byte cooked XA extractions are not suitable.

Linux requirements:
- x86_64 Linux capable of running 32-bit/i386 binaries
- 32-bit glibc runtime
- 32-bit OpenGL/Mesa or vendor OpenGL driver
- OpenGL 3.3 capable GPU/driver
- 32-bit X11 or Wayland runtime libraries
- 32-bit ALSA/PulseAudio/PipeWire runtime libraries

If the game does not launch, run it from a terminal and include:
- distro/version
- GPU/driver
- terminal output
- output of: ldd ./ctr_native
EOF

chmod +x "${PACKAGE_DIR}/ctr_native"

(
    cd "${DIST_DIR}"
    tar -czf "${PACKAGE_NAME}.tar.gz" "${PACKAGE_NAME}"
    sha256sum "${PACKAGE_NAME}.tar.gz" >"${PACKAGE_NAME}.tar.gz.sha256"
)

echo "Wrote ${DIST_DIR}/${PACKAGE_NAME}.tar.gz"
echo "Wrote ${DIST_DIR}/${PACKAGE_NAME}.tar.gz.sha256"
