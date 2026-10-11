#!/usr/bin/env bash
#
# Boot out/Image + out/initramfs.cpio.gz under QEMU and assert that BrightS
# comes up. Exits non-zero if the boot marker never appears.
#
# Used by CI, and works on macOS too (needs qemu-system-aarch64 on PATH).
#
# Usage:
#   ./scripts/boot-test.sh [timeout-seconds]
#
# Environment:
#   QEMU=...   qemu binary (default: qemu-system-aarch64)
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

OUT="${OUT:-$ROOT/out}"
QEMU="${QEMU:-qemu-system-aarch64}"
TIMEOUT="${1:-${TIMEOUT:-180}}"

IMG="$OUT/Image"
INITRD="$OUT/initramfs.cpio.gz"
LOG="${LOG:-$ROOT/build/boot.log}"

MARKER="[selftest] BrightS v0.2c boot ok"

die() { printf '\033[1;31m[x]\033[0m %s\n' "$*" >&2; exit 1; }

command -v "$QEMU" >/dev/null 2>&1 || die "$QEMU not found (apt install qemu-system-arm)"
[ -f "$IMG" ]    || die "missing $IMG — run ./scripts/build.sh kernel"
[ -f "$INITRD" ] || die "missing $INITRD — run ./scripts/build.sh rootfs busybox libs apps initramfs"

mkdir -p "$(dirname "$LOG")"

# `timeout` exists on Linux but not on stock macOS; fall back to a watchdog.
run_qemu() {
    if command -v timeout >/dev/null 2>&1; then
        timeout "$TIMEOUT" "$@"
        return $?
    fi
    local pid watcher rc
    "$@" & pid=$!
    ( sleep "$TIMEOUT"; kill -TERM "$pid" 2>/dev/null ) & watcher=$!
    set +e
    wait "$pid"; rc=$?
    set -e
    kill "$watcher" 2>/dev/null || true
    wait "$watcher" 2>/dev/null || true
    return $rc
}

echo "==> booting $IMG ($(du -h "$IMG" | cut -f1)) + $(du -h "$INITRD" | cut -f1) initramfs, ${TIMEOUT}s budget"

set +e
run_qemu "$QEMU" \
    -M virt -cpu cortex-a72 \
    -kernel "$IMG" \
    -initrd "$INITRD" \
    -append "console=ttyAMA0 rdinit=/System/init brights.selftest" \
    -device virtio-gpu-pci \
    -display none \
    -serial stdio \
    -m 1G -smp 2 \
    -no-reboot \
    >"$LOG" 2>&1
rc=$?
set -e

echo "==> qemu exited with $rc (full log: $LOG)"

# Debian/Ubuntu split QEMU's option ROMs into the ipxe-qemu package, and
# `apt install --no-install-recommends qemu-system-arm` skips it. Without it
# QEMU dies before the guest starts, which otherwise looks like a failed boot.
if grep -q "failed to find romfile" "$LOG"; then
    tail -20 "$LOG" >&2
    die "boot test FAILED: QEMU could not load a PCI option ROM.
     Install ipxe-qemu (it provides /usr/lib/ipxe/qemu/efi-virtio.rom),
     or drop --no-install-recommends so qemu-system-arm pulls it in."
fi

if grep -q "Kernel panic" "$LOG"; then
    tail -40 "$LOG" >&2
    die "boot test FAILED: kernel panic"
fi

if ! grep -qF "$MARKER" "$LOG"; then
    echo "--- last 40 lines of $LOG ---" >&2
    tail -40 "$LOG" >&2
    die "boot test FAILED: marker not found: $MARKER"
fi

if [ "$rc" -eq 124 ]; then
    echo "[!] guest did not power off before the ${TIMEOUT}s timeout" >&2
fi

if grep -q "\[warn\] /dev/dri/card0 missing" "$LOG"; then
    echo "[!] /dev/dri/card0 missing — virtio-gpu did not probe" >&2
fi

echo "==> boot test PASSED"
grep -E "BrightS v0|\[ok\]|\[warn\]|\[fail\]|\[selftest\]" "$LOG" || true
