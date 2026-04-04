#!/usr/bin/env python3
"""
Create a bootable disk image with GPT partition table containing a FAT32 EFI partition.
UEFI requires GPT for native boot (CSM can use MBR).
"""
import struct
import os
import sys

SECTOR_SIZE = 512
CLUSTER_SIZE = 4096
FAT32_RESERVED_CLUSTERS = 2

# GPT partition type GUID for EFI System Partition
EFI_PART_TYPE_GUID = "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"
# Basic Data partition type
BASIC_DATA_TYPE_GUID = "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"

def guid_to_le_bytes(guid_str):
    """Convert GUID string to little-endian bytes"""
    parts = guid_str.split('-')
    data1 = int(parts[0], 16)
    data2 = int(parts[1], 16)
    data3 = int(parts[2], 16)
    data4 = int(parts[3][:2], 16) << 8 | int(parts[3][2:], 16)
    data5 = int(parts[4][0:2], 16) << 40 | int(parts[4][2:4], 16) << 32 | \
            int(parts[4][4:6], 16) << 24 | int(parts[4][6:8], 16) << 16 | \
            int(parts[4][8:10], 16) << 8 | int(parts[4][10:12], 16)
    data4_bytes = bytes([data4 >> 8, data4 & 0xFF])
    data5_bytes = bytes([
        (data5 >> 40) & 0xFF, (data5 >> 32) & 0xFF, (data5 >> 24) & 0xFF,
        (data5 >> 16) & 0xFF, (data5 >> 8) & 0xFF, data5 & 0xFF
    ])
    return struct.pack('<IHH', data1, data2, data3) + data4_bytes + data5_bytes

def crc32(data):
    import zlib
    return zlib.crc32(data) & 0xFFFFFFFF

def create_fat32_fs(total_sectors, efi_data):
    """Create FAT32 filesystem data"""
    efi_size = len(efi_data)
    efi_clusters = (efi_size + CLUSTER_SIZE - 1) // CLUSTER_SIZE
    efi_padded = efi_data + b'\x00' * (efi_clusters * CLUSTER_SIZE - efi_size)

    reserved_sectors = 32
    num_fats = 2
    sectors_per_fat = 32
    root_dir_cluster = 2
    file_data_start_cluster = 5

    fat = [0] * (total_sectors // (CLUSTER_SIZE // SECTOR_SIZE))
    fat[0] = 0x0FFFFFF8
    fat[1] = 0xFFFFFFFF
    for i in range(2, 5):
        fat[i] = 0xFFFFFFFF
    for i in range(file_data_start_cluster, file_data_start_cluster + efi_clusters):
        fat[i] = 0xFFFFFFFF

    def make_short_name_entry(short_name, ext, attr, cluster, size):
        entry = bytearray(32)
        entry[0:8] = short_name.ljust(8).encode('ascii')[:8]
        entry[8:11] = ext.ljust(3).encode('ascii')[:3]
        entry[11] = attr
        entry[20] = (cluster >> 16) & 0xFF
        entry[21] = (cluster >> 24) & 0xFF
        entry[22] = cluster & 0xFF
        entry[23] = (cluster >> 8) & 0xFF
        struct.pack_into('<I', entry, 28, size)
        return bytes(entry)

    def compute_lfn_checksum(short_name):
        name_bytes = (short_name.ljust(8) + "   ")[:11]
        checksum = 0
        for b in name_bytes.encode('ascii'):
            checksum = ((checksum & 1) << 7) + (checksum >> 1) + b
        return checksum & 0xFF

    root_entries = bytearray(CLUSTER_SIZE)
    offset = 0
    root_entries[offset:offset+32] = make_short_name_entry("EFI", "", 0x10, 3, 0)
    offset += 32
    root_entries[offset:offset+32] = make_short_name_entry("BRIGHTS  ", "", 0x08, 0, 0)

    efi_dir_entries = bytearray(CLUSTER_SIZE)
    offset = 0
    efi_dir_entries[offset:offset+32] = make_short_name_entry(".", "", 0x10, 3, 0)
    offset += 32
    efi_dir_entries[offset:offset+32] = make_short_name_entry("..", "", 0x10, 2, 0)
    offset += 32
    efi_dir_entries[offset:offset+32] = make_short_name_entry("BOOT", "", 0x10, 4, 0)

    boot_dir_entries = bytearray(CLUSTER_SIZE)
    offset = 0
    boot_dir_entries[offset:offset+32] = make_short_name_entry(".", "", 0x10, 4, 0)
    offset += 32
    boot_dir_entries[offset:offset+32] = make_short_name_entry("..", "", 0x10, 3, 0)
    offset += 32

    checksum = compute_lfn_checksum("BOOTX64.EFI")
    lfn_entry = bytearray(32)
    lfn_entry[0] = 0x41
    lfn_name = "BOOTX64.EFI"
    name_utf16 = lfn_name.encode('utf-16-le')
    for i in range(5):
        if i * 2 < len(name_utf16):
            lfn_entry[1 + i*2] = name_utf16[i*2]
            lfn_entry[1 + i*2 + 1] = name_utf16[i*2 + 1]
        else:
            lfn_entry[1 + i*2] = 0xFF
            lfn_entry[1 + i*2 + 1] = 0xFF
    for i in range(6):
        idx = 5 + i
        if idx * 2 < len(name_utf16):
            lfn_entry[14 + i*2] = name_utf16[idx*2]
            lfn_entry[14 + i*2 + 1] = name_utf16[idx*2 + 1]
        else:
            lfn_entry[14 + i*2] = 0xFF
            lfn_entry[14 + i*2 + 1] = 0xFF
    for i in range(2):
        idx = 11 + i
        if idx * 2 < len(name_utf16):
            lfn_entry[28 + i*2] = name_utf16[idx*2]
            lfn_entry[28 + i*2 + 1] = name_utf16[idx*2 + 1]
        else:
            lfn_entry[28 + i*2] = 0xFF
            lfn_entry[28 + i*2 + 1] = 0xFF
    lfn_entry[11] = 0x0F
    lfn_entry[26] = checksum
    boot_dir_entries[offset:offset+32] = bytes(lfn_entry)
    offset += 32
    boot_dir_entries[offset:offset+32] = make_short_name_entry("BOOTX64", "EFI", 0x20, file_data_start_cluster, efi_size)

    fat1_data = bytearray(sectors_per_fat * SECTOR_SIZE)
    for i, entry in enumerate(fat):
        struct.pack_into('<I', fat1_data, i * 4, entry & 0x0FFFFFFF)
    fat2_data = bytes(fat1_data)

    boot_sector = bytearray(SECTOR_SIZE)
    boot_sector[0] = 0xEB
    boot_sector[1] = 0x58
    boot_sector[2] = 0x90
    boot_sector[3:11] = b'MSFAT32 '
    struct.pack_into('<H', boot_sector, 11, SECTOR_SIZE)
    boot_sector[13] = CLUSTER_SIZE // SECTOR_SIZE
    struct.pack_into('<H', boot_sector, 14, reserved_sectors)
    boot_sector[16] = num_fats
    struct.pack_into('<H', boot_sector, 17, 0)
    struct.pack_into('<H', boot_sector, 19, 0)
    boot_sector[21] = 0xF8
    struct.pack_into('<H', boot_sector, 22, 0)
    struct.pack_into('<H', boot_sector, 24, 63)
    struct.pack_into('<H', boot_sector, 26, 255)
    struct.pack_into('<I', boot_sector, 28, 0)
    struct.pack_into('<I', boot_sector, 32, total_sectors)
    struct.pack_into('<I', boot_sector, 36, sectors_per_fat)
    struct.pack_into('<H', boot_sector, 40, 0)
    struct.pack_into('<H', boot_sector, 42, 0)
    struct.pack_into('<I', boot_sector, 44, root_dir_cluster)
    struct.pack_into('<H', boot_sector, 48, 1)
    struct.pack_into('<H', boot_sector, 50, 6)
    boot_sector[67] = 0x80
    boot_sector[69] = 0x29
    struct.pack_into('<I', boot_sector, 70, 0x12345678)
    boot_sector[74:85] = b'BRIGHTS EFI '
    boot_sector[85:93] = b'FAT32   '
    boot_sector[510] = 0x55
    boot_sector[511] = 0xAA

    fsinfo = bytearray(SECTOR_SIZE)
    struct.pack_into('<I', fsinfo, 0, 0x41615252)
    struct.pack_into('<I', fsinfo, 484, 0x61417272)
    struct.pack_into('<I', fsinfo, 488, 0xFFFFFFFF)
    struct.pack_into('<I', fsinfo, 492, 2)
    struct.pack_into('<I', fsinfo, 508, 0xAA550000)
    fsinfo[510] = 0x55
    fsinfo[511] = 0xAA

    backup_boot = bytes(boot_sector)

    fs_size = total_sectors * SECTOR_SIZE
    fs = bytearray(fs_size)
    fs[0:SECTOR_SIZE] = boot_sector
    fs[SECTOR_SIZE:SECTOR_SIZE*2] = fsinfo
    fs[SECTOR_SIZE*6:SECTOR_SIZE*7] = backup_boot

    fat1_start = reserved_sectors * SECTOR_SIZE
    fs[fat1_start:fat1_start + len(fat1_data)] = fat1_data

    fat2_start = (reserved_sectors + sectors_per_fat) * SECTOR_SIZE
    fs[fat2_start:fat2_start + len(fat2_data)] = fat2_data

    data_start = (reserved_sectors + num_fats * sectors_per_fat) * SECTOR_SIZE
    fs[data_start:data_start + CLUSTER_SIZE] = root_entries
    fs[data_start + CLUSTER_SIZE:data_start + CLUSTER_SIZE*2] = efi_dir_entries
    fs[data_start + CLUSTER_SIZE*2:data_start + CLUSTER_SIZE*3] = boot_dir_entries
    fs[data_start + CLUSTER_SIZE*3:data_start + CLUSTER_SIZE*3 + len(efi_padded)] = efi_padded

    return bytes(fs)


def create_gpt_disk(output_path, efi_path, disk_size=64*1024*1024):
    """Create disk image with GPT + FAT32 EFI partition"""
    with open(efi_path, 'rb') as f:
        efi_data = f.read()

    # GPT layout: LBA0=MBR, LBA1=GPT header, LBA2-33=partition entries, data starts at LBA34
    # EFI partition: LBA 2048 to LBA 2048+8191 (4MB)
    part_start_lba = 2048
    part_size_sectors = 8192  # 4MB
    part_end_lba = part_start_lba + part_size_sectors - 1

    last_usable_lba = (disk_size // SECTOR_SIZE) - 34 - 1

    # Create FAT32 filesystem
    fs_data = create_fat32_fs(part_size_sectors, efi_data)

    disk = bytearray(disk_size)
    total_sectors = disk_size // SECTOR_SIZE

    # LBA 0: Protective MBR
    mbr = bytearray(SECTOR_SIZE)
    mbr[0] = 0xEB
    mbr[1] = 0x58
    mbr[2] = 0x90
    mbr[3:11] = b'EFI     '
    # Partition entry: type 0xEE (GPT protective), covers entire disk
    mbr[446] = 0x00
    mbr[447] = 0x02
    mbr[448] = 0x00
    mbr[449] = 0xEE
    mbr[450] = 0xFF
    mbr[451] = 0xFF
    mbr[452] = 0xFF
    struct.pack_into('<I', mbr, 454, 1)  # LBA start = 1
    struct.pack_into('<I', mbr, 458, total_sectors - 1)
    mbr[510] = 0x55
    mbr[511] = 0xAA
    disk[0:SECTOR_SIZE] = mbr

    # GPT Header (LBA 1)
    gpt_header = bytearray(SECTOR_SIZE)
    gpt_header[0:8] = b'EFI PART'
    struct.pack_into('<I', gpt_header, 8, 0x00010000)  # Revision 1.0
    struct.pack_into('<I', gpt_header, 12, 92)  # Header size
    struct.pack_into('<I', gpt_header, 16, 0)  # Header CRC32 (computed later)
    struct.pack_into('<I', gpt_header, 20, 0)  # Reserved
    struct.pack_into('<Q', gpt_header, 24, 1)  # Current LBA
    struct.pack_into('<Q', gpt_header, 32, last_usable_lba)  # Backup LBA
    struct.pack_into('<Q', gpt_header, 40, 34)  # First usable LBA
    struct.pack_into('<Q', gpt_header, 48, last_usable_lba)  # Last usable LBA

    # Disk GUID
    import uuid
    disk_guid = uuid.uuid4().bytes_le
    gpt_header[56:72] = disk_guid

    # Partition entry array
    struct.pack_into('<Q', gpt_header, 72, 2)  # Starting LBA of partition entries
    struct.pack_into('<I', gpt_header, 80, 128)  # Number of partition entries
    struct.pack_into('<I', gpt_header, 84, 128)  # Size of each partition entry

    # Compute CRC32 of header (CRC field zeroed)
    header_crc = crc32(bytes(gpt_header))
    struct.pack_into('<I', gpt_header, 16, header_crc)

    disk[SECTOR_SIZE:SECTOR_SIZE*2] = gpt_header

    # Partition Entry Array (LBA 2)
    part_entries = bytearray(128 * 128)  # 128 entries, 128 bytes each

    # Entry 1: EFI System Partition
    entry = bytearray(128)
    entry[0:16] = guid_to_le_bytes(EFI_PART_TYPE_GUID)
    entry[16:32] = uuid.uuid4().bytes_le
    struct.pack_into('<Q', entry, 32, part_start_lba)  # Starting LBA
    struct.pack_into('<Q', entry, 40, part_end_lba)  # Ending LBA
    struct.pack_into('<Q', entry, 48, 0x0000000000000004)  # Attributes: EFI required
    # Partition name (UTF-16, 36 chars = 72 bytes)
    name = "EFI System Partition"
    name_utf16 = name.encode('utf-16-le')
    entry[56:56+len(name_utf16)] = name_utf16

    part_entries[0:128] = entry
    disk[SECTOR_SIZE*2:SECTOR_SIZE*34] = part_entries

    # Write FAT32 filesystem at partition offset
    part_offset = part_start_lba * SECTOR_SIZE
    disk[part_offset:part_offset + len(fs_data)] = fs_data

    # Backup GPT header at end of disk
    backup_header = bytearray(gpt_header)
    struct.pack_into('<Q', backup_header, 24, last_usable_lba)  # Current LBA
    struct.pack_into('<Q', backup_header, 32, 1)  # Backup LBA
    struct.pack_into('<Q', backup_header, 72, last_usable_lba - 33)  # Partition entries start
    struct.pack_into('<I', backup_header, 16, 0)
    backup_crc = crc32(bytes(backup_header))
    struct.pack_into('<I', backup_header, 16, backup_crc)

    backup_offset = last_usable_lba * SECTOR_SIZE
    disk[backup_offset:backup_offset + SECTOR_SIZE] = backup_header

    # Backup partition entries
    backup_part_offset = (last_usable_lba - 33) * SECTOR_SIZE
    disk[backup_part_offset:backup_part_offset + len(part_entries)] = part_entries

    with open(output_path, 'wb') as f:
        f.write(disk)

    print(f"GPT disk image created: {output_path} ({disk_size/1024/1024:.0f}MB)")
    print(f"  Partition: EFI System Partition (LBA {part_start_lba}-{part_end_lba})")
    print(f"  kernel.efi: {len(efi_data)} bytes at EFI/BOOT/BOOTX64.EFI")


if __name__ == '__main__':
    efi_path = sys.argv[1] if len(sys.argv) > 1 else 'build/sys/kernel/kernel.efi'
    output_path = sys.argv[2] if len(sys.argv) > 2 else 'build/brights-disk.img'
    create_gpt_disk(output_path, efi_path)
