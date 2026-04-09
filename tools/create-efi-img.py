#!/usr/bin/env python3
"""
Create a bootable FAT32 EFI System Partition image containing kernel.efi.
No external dependencies - pure Python implementation.
"""
import struct
import os
import sys

SECTOR_SIZE = 512
CLUSTER_SIZE = 4096  # 8 sectors per cluster
FAT32_RESERVED_CLUSTERS = 2

def create_fat32_image(output_path, efi_path, image_size=4*1024*1024):
    """Create a FAT32 filesystem image with kernel.efi in EFI/BOOT/BOOTX64.EFI"""
    
    # Read kernel.efi
    with open(efi_path, 'rb') as f:
        efi_data = f.read()
    efi_size = len(efi_data)
    print(f"kernel.efi size: {efi_size} bytes")
    
    # Pad to cluster boundary
    efi_clusters = (efi_size + CLUSTER_SIZE - 1) // CLUSTER_SIZE
    efi_padded = efi_data + b'\x00' * (efi_clusters * CLUSTER_SIZE - efi_size)
    
    # FAT32 layout:
    # Sector 0: Boot sector (BPB + EBPB)
    # Sectors 1-31: FSInfo sector + reserved
    # FAT1: variable size
    # FAT2: copy of FAT1
    # Root directory: starts at cluster 2
    
    # Calculate FAT size
    # We need enough clusters for our files
    total_sectors = image_size // SECTOR_SIZE
    reserved_sectors = 32  # Boot sector + FSInfo + reserved
    num_fats = 2
    sectors_per_fat = 32  # Enough for 4MB image
    
    # Root directory starts at cluster 2
    root_dir_cluster = 2
    
    # File entries we need:
    # 1. EFI/ (directory)
    # 2. EFI/BOOT/ (directory)  
    # 3. EFI/BOOT/BOOTX64.EFI (file)
    
    # Cluster allocation:
    # Cluster 2: Root directory entries
    # Cluster 3: EFI directory entries
    # Cluster 4: BOOT directory entries
    # Cluster 5+: File data
    
    file_data_start_cluster = 5
    file_data_start_sector = reserved_sectors + num_fats * sectors_per_fat + (file_data_start_cluster - FAT32_RESERVED_CLUSTERS) * (CLUSTER_SIZE // SECTOR_SIZE)
    
    # Build FAT table
    fat = [0] * (total_sectors // (CLUSTER_SIZE // SECTOR_SIZE))
    fat[0] = 0x0FFFFFF8  # Media descriptor
    fat[1] = 0xFFFFFFFF  # Reserved
    fat[2] = 0xFFFFFFFF  # Root dir - end of chain
    fat[3] = 0xFFFFFFFF  # EFI dir - end of chain
    fat[4] = 0xFFFFFFFF  # BOOT dir - end of chain
    
    # File data clusters - end of chain
    for i in range(file_data_start_cluster, file_data_start_cluster + efi_clusters):
        fat[i] = 0xFFFFFFFF  # End of chain
    
    # Build directory entries (32 bytes each)
    def make_lfn_entry(order, name_part, attr=0x0F):
        """Create a Long File Name entry"""
        entry = bytearray(32)
        entry[0] = order
        # UTF-16 name (max 13 chars per LFN entry)
        name_utf16 = name_part.encode('utf-16-le')
        for i in range(0, min(len(name_utf16), 26), 2):
            offset = 1 + i
            if i >= 10:
                offset = 14 + i - 10
            if i >= 20:
                offset = 28 + i - 20
            if offset < 32:
                entry[offset] = name_utf16[i]
                if i + 1 < len(name_utf16):
                    entry[offset + 1] = name_utf16[i + 1]
        entry[11] = attr  # LFN attribute
        entry[26] = 0  # LFN checksum placeholder
        return bytes(entry)
    
    def make_short_name_entry(short_name, ext, attr, cluster, size, checksum=0):
        """Create an 8.3 short name directory entry (FAT32 layout)"""
        entry = bytearray(32)
        # 8-char name padded with spaces
        name_field = short_name.ljust(8).encode('ascii')
        entry[0:8] = name_field[:8]
        # 3-char extension
        ext_field = ext.ljust(3).encode('ascii')
        entry[8:11] = ext_field[:3]
        entry[11] = attr
        # Bytes 20-21: First cluster high (16 bits)
        entry[20] = (cluster >> 16) & 0xFF
        entry[21] = (cluster >> 24) & 0xFF
        # Bytes 22-23: First cluster low (16 bits)
        entry[22] = cluster & 0xFF
        entry[23] = (cluster >> 8) & 0xFF
        # Bytes 28-31: File size (32 bits)
        struct.pack_into('<I', entry, 28, size)
        return bytes(entry)
    
    def compute_lfn_checksum(short_name):
        """Compute LFN checksum for a short name"""
        name_bytes = (short_name.ljust(8) + "   ")[:11]
        checksum = 0
        for b in name_bytes.encode('ascii'):
            checksum = ((checksum & 1) << 7) + (checksum >> 1) + b
        return checksum & 0xFF
    
    # Root directory entries (cluster 2)
    # Entry: EFI directory
    root_entries = bytearray(CLUSTER_SIZE)
    offset = 0
    
    # EFI directory entry
    efi_entry = make_short_name_entry("EFI", "", 0x10, 3, 0)  # Directory, cluster 3
    root_entries[offset:offset+32] = efi_entry
    offset += 32
    
    # Volume label entry
    vol_entry = make_short_name_entry("BRIGHTS  ", "", 0x08, 0, 0)
    root_entries[offset:offset+32] = vol_entry
    offset += 32
    
    # Fill rest with zeros (end marker is 0x00)
    
    # EFI directory entries (cluster 3)
    efi_dir_entries = bytearray(CLUSTER_SIZE)
    offset = 0
    
    # . entry
    dot_entry = make_short_name_entry(".", "", 0x10, 3, 0)
    efi_dir_entries[offset:offset+32] = dot_entry
    offset += 32
    
    # .. entry
    dotdot_entry = make_short_name_entry("..", "", 0x10, 2, 0)
    efi_dir_entries[offset:offset+32] = dotdot_entry
    offset += 32
    
    # BOOT directory entry
    boot_entry = make_short_name_entry("BOOT", "", 0x10, 4, 0)  # Directory, cluster 4
    efi_dir_entries[offset:offset+32] = boot_entry
    offset += 32
    
    # BOOT directory entries (cluster 4)
    boot_dir_entries = bytearray(CLUSTER_SIZE)
    offset = 0
    
    # . entry
    dot_entry = make_short_name_entry(".", "", 0x10, 4, 0)
    boot_dir_entries[offset:offset+32] = dot_entry
    offset += 32
    
    # .. entry
    dotdot_entry = make_short_name_entry("..", "", 0x10, 3, 0)
    boot_dir_entries[offset:offset+32] = dotdot_entry
    offset += 32
    
    # BOOTX64.EFI file entry with LFN
    checksum = compute_lfn_checksum("BOOTX64.EFI")
    
    # LFN entry for "BOOTX64.EFI"
    lfn_entry = bytearray(32)
    lfn_entry[0] = 0x41  # Order=1, last entry flag
    lfn_name = "BOOTX64.EFI"
    name_utf16 = lfn_name.encode('utf-16-le')
    # First 5 chars at offset 1
    for i in range(5):
        if i * 2 < len(name_utf16):
            lfn_entry[1 + i*2] = name_utf16[i*2]
            lfn_entry[1 + i*2 + 1] = name_utf16[i*2 + 1]
        else:
            lfn_entry[1 + i*2] = 0xFF
            lfn_entry[1 + i*2 + 1] = 0xFF
    # Next 6 chars at offset 14
    for i in range(6):
        idx = 5 + i
        if idx * 2 < len(name_utf16):
            lfn_entry[14 + i*2] = name_utf16[idx*2]
            lfn_entry[14 + i*2 + 1] = name_utf16[idx*2 + 1]
        else:
            lfn_entry[14 + i*2] = 0xFF
            lfn_entry[14 + i*2 + 1] = 0xFF
    # Last 2 chars at offset 28
    for i in range(2):
        idx = 11 + i
        if idx * 2 < len(name_utf16):
            lfn_entry[28 + i*2] = name_utf16[idx*2]
            lfn_entry[28 + i*2 + 1] = name_utf16[idx*2 + 1]
        else:
            lfn_entry[28 + i*2] = 0xFF
            lfn_entry[28 + i*2 + 1] = 0xFF
    
    lfn_entry[11] = 0x0F  # LFN attribute
    lfn_entry[26] = checksum
    boot_dir_entries[offset:offset+32] = bytes(lfn_entry)
    offset += 32
    
    # Short name entry: BOOTX64.EFI
    file_entry = make_short_name_entry("BOOTX64", "EFI", 0x20, file_data_start_cluster, efi_size)
    boot_dir_entries[offset:offset+32] = file_entry
    offset += 32
    
    # Build FAT tables
    fat1_data = bytearray(sectors_per_fat * SECTOR_SIZE)
    for i, entry in enumerate(fat):
        if entry == 0:
            continue
        struct.pack_into('<I', fat1_data, i * 4, entry & 0x0FFFFFFF)
    
    fat2_data = bytes(fat1_data)  # Copy
    
    # Build boot sector
    boot_sector = bytearray(SECTOR_SIZE)
    
    # Jump instruction
    boot_sector[0] = 0xEB
    boot_sector[1] = 0x58
    boot_sector[2] = 0x90
    
    # OEM name
    boot_sector[3:11] = b'MSFAT32 '
    
    # BPB
    struct.pack_into('<H', boot_sector, 11, SECTOR_SIZE)       # Bytes per sector
    boot_sector[13] = CLUSTER_SIZE // SECTOR_SIZE              # Sectors per cluster
    struct.pack_into('<H', boot_sector, 14, reserved_sectors)  # Reserved sectors
    boot_sector[16] = num_fats                                 # Number of FATs
    struct.pack_into('<H', boot_sector, 17, 0)                 # Root entries (FAT32 = 0)
    struct.pack_into('<H', boot_sector, 19, 0)                 # Total sectors 16 (FAT32 = 0)
    boot_sector[21] = 0xF8                                     # Media descriptor
    struct.pack_into('<H', boot_sector, 22, 0)                 # FAT size 16 (FAT32 = 0)
    struct.pack_into('<H', boot_sector, 24, 63)                # Sectors per track
    struct.pack_into('<H', boot_sector, 26, 255)               # Number of heads
    struct.pack_into('<I', boot_sector, 28, 0)                 # Hidden sectors
    struct.pack_into('<I', boot_sector, 32, total_sectors)     # Total sectors 32
    
    # FAT32 EBPB
    struct.pack_into('<I', boot_sector, 36, sectors_per_fat)   # FAT size
    struct.pack_into('<H', boot_sector, 40, 0)                 # Extended flags
    struct.pack_into('<H', boot_sector, 42, 0)                 # FS version
    struct.pack_into('<I', boot_sector, 44, root_dir_cluster)  # Root directory cluster
    struct.pack_into('<H', boot_sector, 48, 1)                 # FSInfo sector
    struct.pack_into('<H', boot_sector, 50, 6)                 # Backup boot sector
    # Reserved space
    boot_sector[64:67] = b'EFI   '  # Drive number + reserved
    boot_sector[67] = 0x80          # Drive number (0x80 = hard disk)
    boot_sector[68] = 0             # NT flags
    boot_sector[69] = 0x29          # Extended boot signature
    struct.pack_into('<I', boot_sector, 70, 0x12345678)       # Volume serial number
    boot_sector[74:85] = b'BRIGHTS EFI '                      # Volume label
    boot_sector[85:93] = b'FAT32   '                          # FS type
    
    # Bootstrap code (minimal)
    boot_sector[93:510] = b'\x00' * (510 - 93)
    
    # Boot signature
    boot_sector[510] = 0x55
    boot_sector[511] = 0xAA
    
    # Build FSInfo sector (sector 1)
    fsinfo = bytearray(SECTOR_SIZE)
    struct.pack_into('<I', fsinfo, 0, 0x41615252)   # Lead signature
    struct.pack_into('<I', fsinfo, 484, 0x61417272) # Structure signature
    struct.pack_into('<I', fsinfo, 488, 0xFFFFFFFF) # Free clusters
    struct.pack_into('<I', fsinfo, 492, 2)          # Next free cluster
    struct.pack_into('<I', fsinfo, 508, 0xAA550000) # Trail signature
    fsinfo[510] = 0x55
    fsinfo[511] = 0xAA
    
    # Build backup boot sector (sector 6) - copy of boot sector
    backup_boot = bytes(boot_sector)
    
    # Assemble the image
    image = bytearray(image_size)
    
    # Boot sector
    image[0:SECTOR_SIZE] = boot_sector
    
    # FSInfo sector
    image[SECTOR_SIZE:SECTOR_SIZE*2] = fsinfo
    
    # Reserved sectors (2-5 are zeros, 6 is backup boot)
    image[SECTOR_SIZE*6:SECTOR_SIZE*7] = backup_boot
    
    # FAT1
    fat1_start = reserved_sectors * SECTOR_SIZE
    image[fat1_start:fat1_start + len(fat1_data)] = fat1_data
    
    # FAT2
    fat2_start = (reserved_sectors + sectors_per_fat) * SECTOR_SIZE
    image[fat2_start:fat2_start + len(fat2_data)] = fat2_data
    
    # Root directory (cluster 2)
    root_start = (reserved_sectors + num_fats * sectors_per_fat) * SECTOR_SIZE
    image[root_start:root_start + CLUSTER_SIZE] = root_entries
    
    # EFI directory (cluster 3)
    efi_dir_start = root_start + CLUSTER_SIZE
    image[efi_dir_start:efi_dir_start + CLUSTER_SIZE] = efi_dir_entries
    
    # BOOT directory (cluster 4)
    boot_dir_start = efi_dir_start + CLUSTER_SIZE
    image[boot_dir_start:boot_dir_start + CLUSTER_SIZE] = boot_dir_entries
    
    # File data (cluster 5+)
    file_data_start = boot_dir_start + CLUSTER_SIZE
    image[file_data_start:file_data_start + len(efi_padded)] = efi_padded
    
    # Write image
    with open(output_path, 'wb') as f:
        f.write(image)
    
    print(f"FAT32 EFI image created: {output_path} ({image_size/1024/1024:.0f}MB)")
    print(f"  kernel.efi at EFI/BOOT/BOOTX64.EFI ({efi_size} bytes)")


if __name__ == '__main__':
    efi_path = sys.argv[1] if len(sys.argv) > 1 else 'build/sys/kernel/kernel.efi'
    output_path = sys.argv[2] if len(sys.argv) > 2 else 'build/brights-efi.img'
    create_fat32_image(output_path, efi_path)
