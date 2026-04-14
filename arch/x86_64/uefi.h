#ifndef BRIGHTS_UEFI_H
#define BRIGHTS_UEFI_H

#include <stdint.h>

typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;
typedef uint8_t EFI_BOOL;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;

typedef struct {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
} EFI_GUID;

typedef struct {
  uint64_t Signature;
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t CRC32;
  uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  EFI_STATUS (*Reset)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uint8_t ExtendedVerification);
  EFI_STATUS (*OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, const uint16_t *String);
  EFI_STATUS (*TestString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, const uint16_t *String);
  EFI_STATUS (*QueryMode)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uint64_t ModeNumber, uint64_t *Columns, uint64_t *Rows);
  EFI_STATUS (*SetMode)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uint64_t ModeNumber);
  EFI_STATUS (*SetAttribute)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uint64_t Attribute);
  EFI_STATUS (*ClearScreen)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);
  EFI_STATUS (*SetCursorPosition)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uint64_t Column, uint64_t Row);
  EFI_STATUS (*EnableCursor)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This, uint8_t Visible);
};

typedef struct _EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct {
  EFI_TABLE_HEADER Hdr;
  uint16_t *FirmwareVendor;
  uint32_t FirmwareRevision;
  EFI_HANDLE ConsoleInHandle;
  void *ConIn;
  EFI_HANDLE ConsoleOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
  EFI_HANDLE StandardErrorHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
  void *RuntimeServices;
  EFI_BOOT_SERVICES *BootServices;
  uint64_t NumberOfTableEntries;
  void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

typedef struct {
  uint32_t Type;
  EFI_PHYSICAL_ADDRESS PhysicalStart;
  EFI_VIRTUAL_ADDRESS VirtualStart;
  uint64_t NumberOfPages;
  uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
  EFI_GUID VendorGuid;
  void *VendorTable;
} EFI_CONFIGURATION_TABLE;

struct _EFI_BOOT_SERVICES {
  EFI_TABLE_HEADER Hdr;
  void *RaiseTPL;
  void *RestoreTPL;
  EFI_STATUS (*AllocatePages)(int Type, int MemoryType, uint64_t Pages, EFI_PHYSICAL_ADDRESS *Memory);
  EFI_STATUS (*FreePages)(EFI_PHYSICAL_ADDRESS Memory, uint64_t Pages);
  EFI_STATUS (*GetMemoryMap)(uint64_t *MemoryMapSize, EFI_MEMORY_DESCRIPTOR *MemoryMap, uint64_t *MapKey, uint64_t *DescriptorSize, uint32_t *DescriptorVersion);
  EFI_STATUS (*AllocatePool)(int PoolType, uint64_t Size, void **Buffer);
  EFI_STATUS (*FreePool)(void *Buffer);
  void *CreateEvent;
  void *SetTimer;
  void *WaitForEvent;
  void *SignalEvent;
  void *CloseEvent;
  void *CheckEvent;
  void *InstallProtocolInterface;
  void *ReinstallProtocolInterface;
  void *UninstallProtocolInterface;
  void *HandleProtocol;
  void *Reserved;
  void *RegisterProtocolNotify;
  void *LocateHandle;
  void *LocateDevicePath;
  void *InstallConfigurationTable;
  void *LoadImage;
  void *StartImage;
  void *Exit;
  void *UnloadImage;
  EFI_STATUS (*ExitBootServices)(EFI_HANDLE ImageHandle, uint64_t MapKey);
  // Remaining fields omitted for now.
};

#endif
