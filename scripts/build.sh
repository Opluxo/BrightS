#!/usr/bin/env bash
#
# BrightS build pipeline.
#
# Builds the whole OS from the versions pinned in versions.lock:
#
#   kernel    Linux $KERNEL                 -> out/Image
#   rootfs    /System /Application /User skeleton in build/rootfs
#   busybox   BusyBox $BUSYBOX (static)     -> /System/bin, /System/sbin
#   libdrm    libdrm $LIBDRM (cross-built)  -> /System/usr/lib
#   libs      glibc runtime                 -> /System/lib
#   apps      embedder/test/*.c             -> /System/usr/bin
#   initramfs cpio+gzip of build/rootfs     -> out/initramfs.cpio.gz
#
# Usage:
#   ./scripts/build.sh                 # everything
#   ./scripts/build.sh kernel apps     # only the named stages
#   ./scripts/build.sh clean           # remove build/ and out/
#
# Works both natively on an aarch64 host (the BrightS dev VM) and as a
# cross-build on x86_64 (CI). CROSS_COMPILE is auto-detected; set it
# explicitly if your toolchain uses a different prefix.
#
set -euo pipefail

if [ "${BASH_VERSINFO[0]}" -lt 4 ]; then
    echo "build.sh requires bash 4+ (found $BASH_VERSION). Use the Linux VM." >&2
    exit 1
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# shellcheck source=/dev/null
. "$ROOT/versions.lock"

ARCH=arm64
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
WORK="${WORK:-$ROOT/build}"
DL="$WORK/dl"
SRC="$WORK/src"
RFS="$WORK/rootfs"
OUT="${OUT:-$ROOT/out}"

KERNEL_TARBALL="linux-$KERNEL.tar.xz"
KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/$KERNEL_TARBALL"
BUSYBOX_TARBALL="busybox-$BUSYBOX.tar.bz2"
BUSYBOX_URL="https://busybox.net/downloads/$BUSYBOX_TARBALL"
LIBDRM_TARBALL="libdrm-$LIBDRM.tar.xz"
LIBDRM_URL="https://dri.freedesktop.org/libdrm/$LIBDRM_TARBALL"

# Warn above this size. Not fatal: v0.3 adds the Flutter engine and will
# legitimately be much bigger.
INITRAMFS_WARN_BYTES=$((16 * 1024 * 1024))

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[!]\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31m[x]\033[0m %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------------------
# toolchain
# ---------------------------------------------------------------------------

if [ -z "${CROSS_COMPILE:-}" ]; then
    if [ "$(uname -m)" = "aarch64" ]; then
        CROSS_COMPILE=""
    elif command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
        CROSS_COMPILE="aarch64-linux-gnu-"
    else
        die "no aarch64 toolchain found. Install gcc-aarch64-linux-gnu, or set CROSS_COMPILE."
    fi
fi
CC="${CROSS_COMPILE}gcc"

check_deps() {
    local -a missing=()
    local t
    for t in make cpio gzip tar find readelf file curl meson ninja "$CC"; do
        command -v "$t" >/dev/null 2>&1 || missing+=("$t")
    done

    if [ "${#missing[@]}" -gt 0 ]; then
        warn "missing tools: ${missing[*]}"
        cat >&2 <<'EOF'

Ubuntu / Debian:
  sudo apt-get install -y build-essential gcc-aarch64-linux-gnu \
      flex bison bc libssl-dev libelf-dev cpio xz-utils bzip2 \
      curl file meson ninja-build qemu-system-arm

EOF
        die "install the tools above and re-run."
    fi
    log "toolchain: CC=$CC  ARCH=$ARCH  JOBS=$JOBS"
}

# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

fetch() {
    local url="$1" dest="$2"
    [ -f "$dest" ] && { log "cached: $(basename "$dest")"; return 0; }
    mkdir -p "$(dirname "$dest")"
    log "downloading $(basename "$dest")"
    curl -fsSL --retry 3 --retry-delay 2 -o "$dest.part" "$url" \
        || die "download failed: $url"
    mv "$dest.part" "$dest"
}

# extract <tarball> <destdir> <expected-top-level-name>
extract() {
    local tarball="$1" dest="$2" top="$3"
    mkdir -p "$dest"
    log "extracting $(basename "$tarball")"
    tar -xf "$tarball" -C "$dest"
    [ -d "$dest/$top" ] || die "$tarball did not contain $top/"
    printf '%s\n' "$dest/$top"
}

# Candidate directories holding the toolchain's runtime libraries.
SYSROOT_LIB_DIRS=()
sysroot_dirs() {
    if [ "${#SYSROOT_LIB_DIRS[@]}" -eq 0 ]; then
        local d
        for d in \
            "$(dirname "$("$CC" -print-file-name=libc.so.6)")" \
            "/usr/aarch64-linux-gnu/lib" \
            "/usr/lib/aarch64-linux-gnu" \
            "/lib/aarch64-linux-gnu"
        do
            [ -d "$d" ] && SYSROOT_LIB_DIRS+=("$d")
        done
    fi
    printf '%s\n' "${SYSROOT_LIB_DIRS[@]}"
}

# find_lib <soname> -> absolute path in the toolchain sysroot
find_lib() {
    local name="$1" d p
    p="$("$CC" -print-file-name="$name" 2>/dev/null || true)"
    if [ -n "$p" ] && [ -f "$p" ] && [ "$p" != "$name" ]; then
        printf '%s\n' "$p"; return 0
    fi
    while IFS= read -r d; do
        if [ -e "$d/$name" ]; then
            printf '%s\n' "$d/$name"; return 0
        fi
    done < <(sysroot_dirs)
    return 1
}

# needed_libs <elf> -> one NEEDED soname per line
needed_libs() {
    local out
    out="$(readelf -d "$1" 2>/dev/null || true)"
    sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p' <<< "$out"
}

# The glibc family lives in /System/lib and is staged separately, with the
# loader path baked in at link time, so it is never pulled in transitively.
is_glibc_family() {
    case "$1" in
        libc.so*|ld-linux*|libpthread.so*|libdl.so*|librt.so*|libm.so*|libgcc_s.so*) return 0 ;;
        *) return 1 ;;
    esac
}

# copy_lib_deps <destdir> <elf>
# Copies the transitive dependency closure of <elf> into <destdir>.
copy_lib_deps() {
    local dest="$1" elf="$2"
    local -a queue=()
    local -A seen=()
    local name path dep

    mkdir -p "$dest"
    while IFS= read -r dep; do
        [ -n "$dep" ] && queue+=("$dep")
    done < <(needed_libs "$elf")

    while [ "${#queue[@]}" -gt 0 ]; do
        name="${queue[0]}"; queue=("${queue[@]:1}")
        [ -n "${seen[$name]:-}" ] && continue
        seen[$name]=1
        is_glibc_family "$name" && continue

        path="$(find_lib "$name")" || { warn "library not found, skipping: $name"; continue; }
        cp -Lf "$path" "$dest/$name"
        log "  + $name"

        while IFS= read -r dep; do
            [ -n "$dep" ] && queue+=("$dep")
        done < <(needed_libs "$path")
    done
}

# ---------------------------------------------------------------------------
# stages
# ---------------------------------------------------------------------------

# apply_kernel_fragment <kernel-src-dir>
# Applies configs/kernel.fragment on top of an existing .config using the
# kernel's own scripts/config. The file is passed explicitly so this does not
# depend on the caller's working directory.
apply_kernel_fragment() {
    local k="$1"
    local cfg="$k/scripts/config"
    [ -x "$cfg" ] || die "not a kernel source tree: $k"

    local line opt
    while IFS= read -r line || [ -n "$line" ]; do
        line="${line%%#*}"
        line="$(printf '%s' "$line" | tr -d '[:space:]')"
        [ -z "$line" ] && continue
        opt="${line%%=*}"
        case "$line" in
            CONFIG_*=y) "$cfg" --file "$k/.config" --enable  "$opt" ;;
            CONFIG_*=n) "$cfg" --file "$k/.config" --disable "$opt" ;;
            *) die "invalid line in configs/kernel.fragment: $line" ;;
        esac
    done < "$ROOT/configs/kernel.fragment"
}

stage_kernel() {
    local k
    fetch "$KERNEL_URL" "$DL/$KERNEL_TARBALL"
    if [ -d "$SRC/linux-$KERNEL" ]; then
        k="$SRC/linux-$KERNEL"
        log "kernel: reusing $k"
    else
        k="$(extract "$DL/$KERNEL_TARBALL" "$SRC" "linux-$KERNEL")"
    fi

    cd "$k"
    log "kernel: defconfig"
    make ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" defconfig >/dev/null

    log "kernel: applying configs/kernel.fragment"
    apply_kernel_fragment "$k"

    make ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" olddefconfig >/dev/null

    # Fail loudly if the fragment did not take effect. A symbol that does not
    # exist (or whose dependencies are unmet) is silently dropped by kconfig,
    # which is exactly how a "hardened" kernel ends up without the feature.
    local expect
    for expect in \
        "CONFIG_DRM=y" \
        "CONFIG_DRM_VIRTIO_GPU=y" \
        "CONFIG_DEVTMPFS=y" \
        "CONFIG_BLK_DEV_INITRD=y" \
        "CONFIG_USER_NS=y" \
        "CONFIG_SECURITY_LANDLOCK=y" \
        "# CONFIG_ARM64_64K_PAGES is not set"
    do
        grep -qxF "$expect" .config || die "kernel config missing: $expect"
    done
    log "kernel: config verified"

    log "kernel: building Image (-j$JOBS)"
    make ARCH="$ARCH" CROSS_COMPILE="$CROSS_COMPILE" -j"$JOBS" Image

    mkdir -p "$OUT"
    cp -f arch/arm64/boot/Image "$OUT/Image"
    cd "$ROOT"
    log "kernel: -> $OUT/Image ($(du -h "$OUT/Image" | cut -f1))"
}

stage_rootfs() {
    # Defensive: never rm -rf a path we did not derive ourselves.
    case "$RFS" in
        "$WORK"/rootfs) ;;
        *) die "refusing to clean unexpected rootfs path: $RFS" ;;
    esac

    log "rootfs: assembling skeleton in $RFS"
    rm -rf "$RFS"
    mkdir -p "$RFS"

    # Hand-written overlay files from rootfs/ (currently just System/init).
    # Everything the build generates is laid on top of this afterwards, so the
    # generated directories are skipped entirely -- both the directory and its
    # contents, otherwise `cp -a dir existing-dir` would nest a copy inside it.
    local f rel
    while IFS= read -r -d '' f; do
        rel="${f#"$ROOT/rootfs/"}"
        case "$rel" in
            System/bin|System/bin/*|System/sbin|System/sbin/*|System/lib|System/lib/*|\
            System/usr/bin|System/usr/bin/*|System/usr/sbin|System/usr/sbin/*|\
            System/usr/lib|System/usr/lib/*)
                continue ;;
        esac
        if [ -d "$f" ] && [ ! -L "$f" ]; then
            # find enumerates children individually, so just create the dir.
            mkdir -p "$RFS/$rel"
        else
            mkdir -p "$RFS/$(dirname "$rel")"
            cp -a "$f" "$RFS/$rel"
        fi
    done < <(find "$ROOT/rootfs" -mindepth 1 -print0)

    mkdir -p \
        "$RFS/System/bin" "$RFS/System/sbin" "$RFS/System/lib" \
        "$RFS/System/usr/bin" "$RFS/System/usr/lib" "$RFS/System/usr/share" \
        "$RFS/Application" \
        "$RFS/User/Documents" "$RFS/User/Pictures" "$RFS/User/Music" \
        "$RFS/User/Videos" "$RFS/User/Downloads" \
        "$RFS/dev" "$RFS/proc" "$RFS/sys" "$RFS/run" "$RFS/tmp"

    [ -f "$RFS/System/init" ] || die "rootfs/System/init missing from the overlay"
    chmod 0755 "$RFS/System/init"
    log "rootfs: skeleton ready"
}

stage_busybox() {
    local b ftype
    fetch "$BUSYBOX_URL" "$DL/$BUSYBOX_TARBALL"
    if [ -d "$SRC/busybox-$BUSYBOX" ]; then
        b="$SRC/busybox-$BUSYBOX"
        log "busybox: reusing $b"
    else
        b="$(extract "$DL/$BUSYBOX_TARBALL" "$SRC" "busybox-$BUSYBOX")"
    fi

    cd "$b"
    log "busybox: installing pinned config"
    cp -f "$ROOT/configs/busybox.config" .config
    # No prompts: the pinned config is complete for this version, and anything
    # new falls back to its default.
    make CROSS_COMPILE="$CROSS_COMPILE" oldconfig </dev/null >/dev/null

    grep -qxF "CONFIG_STATIC=y" .config || die "busybox: CONFIG_STATIC did not stick"
    grep -qxF "# CONFIG_TC is not set" .config || die "busybox: CONFIG_TC must be off"

    log "busybox: building (-j$JOBS)"
    make CROSS_COMPILE="$CROSS_COMPILE" -j"$JOBS" >/dev/null

    log "busybox: installing into $RFS/System"
    make CROSS_COMPILE="$CROSS_COMPILE" CONFIG_PREFIX="$RFS/System" install >/dev/null
    cd "$ROOT"

    ftype="$(file -b "$RFS/System/bin/busybox")"
    case "$ftype" in
        *"statically linked"*) ;;
        *) die "busybox is not statically linked: $ftype" ;;
    esac
    log "busybox: $(du -h "$RFS/System/bin/busybox" | cut -f1) static binary"
}

stage_libdrm() {
    local d cross prefix so
    fetch "$LIBDRM_URL" "$DL/$LIBDRM_TARBALL"
    if [ -d "$SRC/libdrm-$LIBDRM" ]; then
        d="$SRC/libdrm-$LIBDRM"
        log "libdrm: reusing $d"
    else
        d="$(extract "$DL/$LIBDRM_TARBALL" "$SRC" "libdrm-$LIBDRM")"
    fi

    prefix="$WORK/libdrm-install"
    cross="$WORK/libdrm-cross.txt"
    mkdir -p "$WORK"

    # Only the DRM core is needed. The per-driver backends are dead weight and
    # intel in particular pulls in host-side libpciaccess.
    cat > "$cross" <<EOF
[binaries]
c = '$CC'
ar = '${CROSS_COMPILE}ar'
strip = '${CROSS_COMPILE}strip'

[host_machine]
system = 'linux'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
EOF

    log "libdrm: configuring (meson cross build)"
    rm -rf "$d/build-cross"
    meson setup "$d/build-cross" "$d" \
        --cross-file "$cross" \
        --prefix=/System/usr \
        --libdir=lib \
        --buildtype=release \
        -Dtests=false \
        -Dintel=disabled \
        -Dradeon=disabled \
        -Damdgpu=disabled \
        -Dnouveau=disabled

    log "libdrm: building"
    ninja -C "$d/build-cross" >/dev/null

    rm -rf "$prefix"
    DESTDIR="$prefix" ninja -C "$d/build-cross" install >/dev/null

    mkdir -p "$RFS/System/usr/lib"
    # Search recursively: the install libdir depends on meson's defaults.
    so="$(find "$prefix" -name 'libdrm.so.2.*' ! -type l -print -quit 2>/dev/null || true)"
    [ -n "$so" ] || die "libdrm: libdrm.so.2 was not produced"
    cp -f "$so" "$RFS/System/usr/lib/libdrm.so.2"

    copy_lib_deps "$RFS/System/usr/lib" "$so"
    log "libdrm: -> $RFS/System/usr/lib/libdrm.so.2"
}

stage_libs() {
    log "glibc: staging runtime from the toolchain sysroot"
    mkdir -p "$RFS/System/lib"

    local name path
    for name in libc.so.6 libm.so.6 ld-linux-aarch64.so.1; do
        path="$(find_lib "$name")" || die "cannot locate $name in the toolchain sysroot"
        cp -Lf "$path" "$RFS/System/lib/$name"
        log "  + $name"
    done

    [ -f "$RFS/System/lib/ld-linux-aarch64.so.1" ] || die "loader missing"
    log "glibc: ready"
}

stage_apps() {
    local out needed prefix hdr drmh libso
    prefix="$WORK/libdrm-install"
    out="$RFS/System/usr/bin"

    # libdrm installs the public entry points (xf86drm.h, xf86drmMode.h) into
    # include/ but the DRM ioctl headers (drm.h, drm_mode.h) into
    # include/libdrm/, and xf86drm.h includes <drm.h>. Discover both rather
    # than assuming the layout.
    hdr="$(find "$prefix" -name xf86drm.h -print -quit 2>/dev/null || true)"
    drmh="$(find "$prefix" -name drm.h -print -quit 2>/dev/null || true)"
    libso="$(find "$prefix" -name 'libdrm.so' -print -quit 2>/dev/null || true)"
    [ -n "$hdr" ]   || die "libdrm headers missing; run the libdrm stage first"
    [ -n "$libso" ] || die "libdrm.so missing; run the libdrm stage first"

    local -a incflags=(-I"$(dirname "$hdr")")
    [ -n "$drmh" ] && incflags+=(-I"$(dirname "$drmh")")
    mkdir -p "$out"

    # BrightS has no /lib and no FHS: loader and rpath are absolute paths.
    # shellcheck disable=SC2054  # commas inside -Wl, flags are not separators
    local -a ldflags=(
        -Wl,--dynamic-linker=/System/lib/ld-linux-aarch64.so.1
        -Wl,-rpath=/System/usr/lib:/System/lib
        -Wl,--enable-new-dtags
    )

    log "apps: drm-test"
    "$CC" -O2 -Wall -o "$out/drm-test" "$ROOT/embedder/test/drm-test.c" \
        "${incflags[@]}" "${ldflags[@]}" -L"$(dirname "$libso")" -ldrm

    log "apps: hello"
    "$CC" -O2 -Wall -o "$out/hello" "$ROOT/embedder/test/hello.c" \
        "${ldflags[@]}"

    local bin
    for bin in drm-test hello; do
        needed="$(needed_libs "$out/$bin")"
        [ -n "$needed" ] || die "$bin has no NEEDED entries (not dynamically linked?)"
        log "  + $bin ($(du -h "$out/$bin" | cut -f1))"
    done
}

stage_initramfs() {
    mkdir -p "$OUT"
    log "initramfs: packing $RFS"
    ( cd "$RFS" && find . | cpio -o -H newc 2>/dev/null | gzip -9 ) > "$OUT/initramfs.cpio.gz"

    local size
    size=$(wc -c < "$OUT/initramfs.cpio.gz")
    log "initramfs: -> $OUT/initramfs.cpio.gz ($((size / 1024))K)"
    if [ "$size" -gt "$INITRAMFS_WARN_BYTES" ]; then
        warn "initramfs is larger than $((INITRAMFS_WARN_BYTES / 1024 / 1024))MB."
        warn "check $RFS/System/usr/lib for stray Mesa/LLVM libraries."
    fi
}

stage_clean() {
    log "removing $WORK and $OUT"
    rm -rf "$WORK" "$OUT"
}

usage() {
    cat <<EOF
BrightS build pipeline.

Usage:
  ./scripts/build.sh                 build everything
  ./scripts/build.sh <stage> ...     build only the named stages
  ./scripts/build.sh clean           remove build/ and out/

Stages:
  kernel      Linux $KERNEL        -> out/Image
  rootfs      /System /Application /User skeleton
  busybox     BusyBox $BUSYBOX (static)
  libdrm      libdrm $LIBDRM (cross-built)
  libs        glibc runtime -> /System/lib
  apps        embedder/test/*.c -> /System/usr/bin
  initramfs   cpio+gzip -> out/initramfs.cpio.gz

Environment:
  CROSS_COMPILE=aarch64-linux-gnu-   toolchain prefix (auto-detected)
  JOBS=8                             parallel make jobs
  OUT=path                           artifact directory (default: out/)
  WORK=path                          build directory (default: build/)
EOF
}

# ---------------------------------------------------------------------------
# entry point
# ---------------------------------------------------------------------------

ALL_STAGES=(kernel rootfs busybox libdrm libs apps initramfs)

main() {
    # The rootfs stage rm -rf's $RFS, so refuse any WORK that could point it
    # at tracked files.
    if [ -z "$WORK" ] || [ "$WORK" = "/" ] || [ "$WORK" = "$ROOT" ]; then
        die "refusing to use WORK=$WORK (set WORK to a scratch directory)"
    fi

    local -a stages=()
    if [ "$#" -eq 0 ]; then
        stages=("${ALL_STAGES[@]}")
    else
        local s
        for s in "$@"; do
            case "$s" in
                all)      stages+=("${ALL_STAGES[@]}") ;;
                clean)    stage_clean; return 0 ;;
                -h|--help|help) usage; return 0 ;;
                kernel|rootfs|busybox|libdrm|libs|apps|initramfs) stages+=("$s") ;;
                *) die "unknown stage: $s (see --help)" ;;
            esac
        done
    fi

    check_deps
    mkdir -p "$WORK" "$DL" "$SRC" "$OUT"

    local s
    for s in "${stages[@]}"; do
        "stage_$s"
    done
    log "done."
}

main "$@"
