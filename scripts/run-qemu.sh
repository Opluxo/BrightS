#!/usr/bin/env bash
#
# Run the built BrightS image interactively under QEMU.
#
#   ./scripts/run-qemu.sh                 # window (cocoa on macOS, gtk elsewhere)
#   BRIGHTS_DISPLAY=vnc ./scripts/run-qemu.sh
#   BRIGHTS_DISPLAY=none ./scripts/run-qemu.sh   # serial console only
#
# Environment:
#   QEMU=...              qemu binary (default: qemu-system-aarch64)
#   BRIGHTS_DISPLAY=...   cocoa | gtk | sdl | vnc | none
#   BRIGHTS_MEM=1G        guest RAM
#   BRIGHTS_SMP=2         guest CPUs
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${OUT:-$ROOT/out}"

QEMU="${QEMU:-qemu-system-aarch64}"
MEM="${BRIGHTS_MEM:-1G}"
SMP="${BRIGHTS_SMP:-2}"

if [ -z "${BRIGHTS_DISPLAY:-}" ]; then
    case "$(uname -s)" in
        Darwin) BRIGHTS_DISPLAY=cocoa ;;
        *)      BRIGHTS_DISPLAY=gtk ;;
    esac
fi

die() { printf '\033[1;31m[x]\033[0m %s\n' "$*" >&2; exit 1; }

command -v "$QEMU" >/dev/null 2>&1 || die "$QEMU not found (apt install qemu-system-arm)"
[ -f "$OUT/Image" ]            || die "missing $OUT/Image — run ./scripts/build.sh kernel"
[ -f "$OUT/initramfs.cpio.gz" ] || die "missing $OUT/initramfs.cpio.gz — run ./scripts/build.sh"

echo "==> $QEMU  display=$BRIGHTS_DISPLAY  mem=$MEM  smp=$SMP"
echo "==> serial console follows; Ctrl-A X quits QEMU"

set -- \
    -M virt -cpu cortex-a72 \
    -kernel "$OUT/Image" \
    -initrd "$OUT/initramfs.cpio.gz" \
    -append "console=ttyAMA0 rdinit=/System/init" \
    -device virtio-gpu-pci \
    -serial stdio \
    -m "$MEM" -smp "$SMP"

if [ "$BRIGHTS_DISPLAY" = "none" ]; then
    exec "$QEMU" "$@" -display none
else
    exec "$QEMU" "$@" -display "$BRIGHTS_DISPLAY"
fi
