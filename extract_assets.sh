#!/usr/bin/env bash
set -euo pipefail

# Extract CTR (NTSC-U) game assets from a retail disc image (.bin/.cue or .iso).
# XA audio files are extracted as raw 2352-byte sectors (required by ctr-native).
# All other files are extracted as standard 2048-byte ISO data.
#
# Usage:
#   ./extract_assets.sh /path/to/CTR.bin            (will look for matching .cue)
#   ./extract_assets.sh /path/to/CTR.iso
#   ./extract_assets.sh /path/to/CTR.bin ./assets   (custom output dir)

die() { echo "[ERROR] $*" >&2; exit 1; }
info() { echo "[INFO] $*"; }

INPUT="${1:-}"
OUTPUT_DIR="${2:-assets}"

[ -z "$INPUT" ] && die "Usage: $0 <disc-image.bin|.iso> [output-dir]"
[ -f "$INPUT" ] || die "File not found: $INPUT"

# --- Resolve ISO path ---

ISO_PATH=""
CLEANUP_ISO=0

ext="${INPUT##*.}"
ext_lower="$(echo "$ext" | tr '[:upper:]' '[:lower:]')"

if [ "$ext_lower" = "iso" ]; then
    ISO_PATH="$INPUT"
elif [ "$ext_lower" = "bin" ]; then
    # Need bchunk to convert bin/cue → iso
    CUE_PATH="${INPUT%.*}.cue"
    [ -f "$CUE_PATH" ] || CUE_PATH="${INPUT%.*}.CUE"
    [ -f "$CUE_PATH" ] || die "Cannot find .cue file for: $INPUT (tried ${INPUT%.*}.cue)"

    if ! command -v bchunk &>/dev/null; then
        info "bchunk not found, building from source..."
        BCHUNK_DIR="$(mktemp -d)"
        git clone --depth 1 https://github.com/hessu/bchunk.git "$BCHUNK_DIR/src" 2>/dev/null
        make -C "$BCHUNK_DIR/src" -s
        BCHUNK="$BCHUNK_DIR/src/bchunk"
    else
        BCHUNK="bchunk"
    fi

    # bchunk takes an output PREFIX and appends "01.iso", "02.iso", etc.
    ISO_PREFIX="$(mktemp -d)/ctr_track"
    CLEANUP_ISO=1
    info "Converting BIN/CUE → ISO..."
    "$BCHUNK" "$INPUT" "$CUE_PATH" "$ISO_PREFIX" >/dev/null 2>&1 || true

    ISO_PATH="${ISO_PREFIX}01.iso"
    [ -f "$ISO_PATH" ] || die "bchunk conversion failed (expected ${ISO_PATH})"
    info "ISO created: $ISO_PATH"
else
    die "Unsupported format: .$ext (expected .bin or .iso)"
fi

# --- Verify it's a valid CTR disc ---

if ! command -v isoinfo &>/dev/null; then
    die "isoinfo not found. Install genisoimage or cdrtools."
fi

isoinfo -i "$ISO_PATH" -l >/dev/null 2>&1 || die "Not a valid ISO 9660 image: $ISO_PATH"
isoinfo -i "$ISO_PATH" -l 2>/dev/null | grep -q "BIGFILE.BIG" || die "BIGFILE.BIG not found — not a CTR disc image"

info "Valid CTR disc image detected."

# --- Extract standard files (2048-byte sectors) ---

info "Extracting standard assets to $OUTPUT_DIR/ ..."
mkdir -p "$OUTPUT_DIR/SOUNDS"

isoinfo -i "$ISO_PATH" -x "/BIGFILE.BIG;1" > "$OUTPUT_DIR/BIGFILE.BIG"
isoinfo -i "$ISO_PATH" -x "/SOUNDS/KART.HWL;1" > "$OUTPUT_DIR/SOUNDS/KART.HWL"
isoinfo -i "$ISO_PATH" -x "/TEST.STR;1" > "$OUTPUT_DIR/TEST.STR"

# --- Extract XA files in raw 2352-byte sectors from the original BIN ---
# isoinfo strips CD subheaders, making XA audio unreadable.
# We must pull raw sectors directly from the disc image.

info "Extracting XA audio files (raw 2352-byte sectors)..."

python3 - "$INPUT" "$ISO_PATH" "$OUTPUT_DIR" << 'PYEOF'
import sys, os, subprocess, struct

bin_path = sys.argv[1]
iso_path = sys.argv[2]
output_dir = sys.argv[3]

RAW_SECTOR = 2352
ISO_SECTOR = 2048

# If input is ISO (no raw sectors available), fall back to isoinfo extraction
# with a warning. Raw extraction only works from .bin files.
input_ext = os.path.splitext(bin_path)[1].lower()
use_raw = input_ext == ".bin"

# Parse XA file locations from isoinfo
result = subprocess.run(['isoinfo', '-i', iso_path, '-l'], capture_output=True, text=True)
lines = result.stdout.splitlines()

xa_files = []
current_dir = ""
for line in lines:
    if line.startswith("Directory listing of"):
        current_dir = line.split("of ")[-1].strip()
    elif ";1" in line and "/XA" in current_dir:
        parts = line.split()
        name = parts[-1].split(';')[0]
        if not name.upper().endswith(".XA"):
            continue
        size = None
        for p in parts:
            if p.isdigit() and int(p) > 100:
                size = int(p)
                break
        if size is None:
            continue
        bracket_start = line.index('[')
        bracket_end = line.index(']')
        extent = int(line[bracket_start+1:bracket_end].strip().split()[0])
        filepath = current_dir + name
        xa_files.append((filepath, extent, size))

if not xa_files:
    print("[WARN] No XA files found in disc image")
    sys.exit(0)

for filepath, lba, iso_size in xa_files:
    rel_path = filepath.lstrip('/')
    out_path = os.path.join(output_dir, rel_path)
    os.makedirs(os.path.dirname(out_path), exist_ok=True)

    num_sectors = (iso_size + ISO_SECTOR - 1) // ISO_SECTOR

    if use_raw:
        with open(bin_path, 'rb') as f:
            raw_data = bytearray()
            for s in range(num_sectors):
                f.seek((lba + s) * RAW_SECTOR)
                raw_data.extend(f.read(RAW_SECTOR))
        with open(out_path, 'wb') as out:
            out.write(raw_data)
        actual_size = len(raw_data)
    else:
        # Fallback: extract via isoinfo (no raw sectors — voices may not work)
        iso_name = "/" + rel_path.replace("/", "/") + ";1"
        result = subprocess.run(['isoinfo', '-i', iso_path, '-x', iso_name],
                                capture_output=True)
        with open(out_path, 'wb') as out:
            out.write(result.stdout)
        actual_size = len(result.stdout)

    print(f"  {rel_path} ({actual_size} bytes, {num_sectors} sectors" +
          (" raw" if use_raw else " ISO-MODE WARNING: voices may not work") + ")")

if not use_raw:
    print("\n[WARN] Input was .iso — XA files extracted without raw CD headers.")
    print("       Voices/music may not play. Use the original .bin for full audio.")
PYEOF

# --- Also extract ENG.XNF (small index file, standard extraction is fine) ---

mkdir -p "$OUTPUT_DIR/XA"
isoinfo -i "$ISO_PATH" -x "/XA/ENG.XNF;1" > "$OUTPUT_DIR/XA/ENG.XNF"

# --- Cleanup ---

[ "$CLEANUP_ISO" -eq 1 ] && rm -f "$ISO_PATH"

# --- Verify ---

info "Verifying extracted assets..."
MISSING=0
for f in BIGFILE.BIG SOUNDS/KART.HWL TEST.STR XA/ENG.XNF; do
    if [ ! -s "$OUTPUT_DIR/$f" ]; then
        echo "  MISSING: $f"
        MISSING=1
    fi
done

XA_COUNT=$(find "$OUTPUT_DIR/XA" -name "*.XA" | wc -l)
if [ "$XA_COUNT" -lt 28 ]; then
    echo "  WARNING: Only $XA_COUNT XA files found (expected 29)"
    MISSING=1
fi

if [ "$MISSING" -eq 0 ]; then
    info "All assets extracted successfully ($XA_COUNT XA files)."
    info "Output directory: $OUTPUT_DIR/"
    echo ""
    echo "To use with ctr-native, ensure assets/ is next to the ctr_native binary:"
    echo "  ln -sf \$(realpath $OUTPUT_DIR) build/assets"
else
    die "Some assets are missing — check disc image integrity."
fi
