# Build and Run

This project builds a UEFI x86_64 kernel image and runs it in QEMU.

## Requirements
- CMake >= 3.20
- clang + lld-link
- QEMU
- OVMF UEFI firmware files:
  - `/usr/share/OVMF/OVMF_CODE.fd`
  - `/usr/share/OVMF/OVMF_VARS.fd`
  - `/usr/share/OVMF/x64/OVMF_CODE.4m.fd`
  - `/usr/share/OVMF/x64/OVMF_VARS.4m.fd`
  - `/usr/share/edk2/x64/OVMF_CODE.4m.fd`
  - `/usr/share/edk2/x64/OVMF_VARS.4m.fd`
  - `/usr/share/edk2-ovmf/x64/OVMF_CODE.4m.fd`
  - `/usr/share/edk2-ovmf/x64/OVMF_VARS.4m.fd`
- `xorriso` (only needed for ISO build)

## Build
```bash
cmake -S . -B build
cmake --build build
```

## Run (QEMU)
```bash
./scripts/run-qemu.sh
```

Expected serial output includes lines like:
```text
nvme: init ok
# or
ahci: init ok
# or
ramdisk: fallback
btrfs: mount ok
# or
btrfs: mount failed
```

## Build UEFI ISO
```bash
./scripts/make-iso.sh
```

## Troubleshooting
- `kernel.efi not found`: run `cmake --build build` first.
- OVMF file missing: install your distro `ovmf` or `edk2-ovmf` package, or adjust script paths.
- `xorriso: command not found`: install `xorriso`.
