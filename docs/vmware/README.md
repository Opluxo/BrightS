# VMware Support in BrightS

## Overview

BrightS includes VMware backdoor support for running inside VMware virtual machines.

## Status

- ✅ VMware detection during kernel boot
- ✅ VMware backdoor I/O port communication
- ✅ VMware time synchronization
- ⏳ VMware drag-and-drop / shared folders (planned)

## How It Works

1. During boot, the kernel checks for VMware backdoor availability via I/O port magic
2. If VMware is detected, it initializes the backdoor interface
3. VMware-specific features like time synchronization are enabled

## Files

- `kernel/core/vmware.c` — backdoor implementation
- `kernel/core/vmware.h` — interface definitions
- `kernel/core/kernel_main.c` — initialization call

## Testing

Run in VMware Workstation/Player:
1. Create a new VM
2. Attach the built ISO
3. Boot the VM
4. Check serial output for "vmware: backdoor initialized"

## Notes

- Backdoor only active in VMware environments
- In QEMU or physical hardware: "vmware: backdoor not available"
- The ISO image supports both QEMU and VMware

*Last updated: 2026-05-29*
