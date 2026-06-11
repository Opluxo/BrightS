# BrightS System Suite License Compatibility

This document explains the license compatibility across the BrightS System Suite projects.

## License Overview

| Project | License | SPDX | Notes |
|---------|---------|------|-------|
| BrightS | GPL-2.0-only WITH Linux-syscall-note exception | GPL-2.0-only | Special exception for userspace programs |
| BrightS_Package_Manager | GPL-3.0-or-later | GPL-3.0-or-later | Standard GPL v3 |
| SuperFetch | GPL-3.0-or-later | GPL-3.0-or-later | Standard GPL v3 |

## Compatibility Matrix

### Combining Code

| From/To | BrightS | BSPM | SuperFetch |
|---------|---------|------|------------|
| BrightS | ✅ | ✅ | ✅ |
| BSPM | ❌ | ✅ | ✅ |
| SuperFetch | ❌ | ✅ | ✅ |

**Notes:**
- GPL-2.0 code cannot be combined with GPL-3.0 code
- The special exception in BrightS allows userspace programs to link with the kernel
- All projects can use GPL-3.0 libraries and tools

### Distribution

- BrightS binaries can be distributed under GPL-2.0 terms
- BSPM and SuperFetch must follow GPL-3.0 distribution terms
- Combined distributions must comply with the most restrictive license (GPL-3.0)

## Recommended Practices

### For Contributors

1. **New code**: Use GPL-3.0-or-later for compatibility
2. **BrightS kernel**: Follow existing GPL-2.0 + exception pattern
3. **Check compatibility**: Before combining code from different projects

### For Users

1. **Source access**: All projects provide source code access
2. **Modification rights**: You can modify and redistribute under GPL terms
3. **No additional restrictions**: No restrictions beyond GPL requirements

## License Texts

### BrightS Special Exception

```
EXCEPTION TO THE GPL VERSION 2

As a special exception, permission is granted to dynamically link userspace
programs with this kernel and to distribute the resulting executable,
provided that all of the following conditions are met:

  (a) The userspace program communicates with the kernel solely through
      normal system calls, standard ABI interfaces, or standard pseudo-filesystems
      (e.g., /proc, /sys, /dev);

  (b) No part of the kernel source code (including header files that expose
      internal kernel data structures or implementation details) is included
      in the userspace program;

  (c) The userspace program does not contain derivative works of the kernel.

This exception does not override any other obligations under the GNU General
Public License version 2. It applies only to the act of linking and distributing
userspace programs that meet the above criteria.
```

### GPL Version Compatibility

- **GPL-2.0-only**: Code must use exactly GPL-2.0 (no later versions)
- **GPL-3.0-or-later**: Code can use GPL-3.0 or any later GPL version
- **AGPL-3.0**: Network service requirement (dropped for SuperFetch)

## Migration Notes

- SuperFetch was originally AGPL-3.0 but changed to GPL-3.0 for better compatibility
- BrightS maintains GPL-2.0 for kernel compatibility reasons
- New projects in the suite should use GPL-3.0-or-later

## Legal Compliance

All projects in the BrightS System Suite comply with:
- Free Software Foundation licensing guidelines
- Open Source Initiative approved licenses
- SPDX license identifiers

For legal questions, consult with a qualified legal professional familiar with open source licensing.