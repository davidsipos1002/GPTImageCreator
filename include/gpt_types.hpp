#pragma once

#include "cal_types.h"

/**
  Returns a 16-bit signature built from 2 ASCII characters.

  This macro returns a 16-bit value built from the two ASCII characters specified
  by A and B.

  @param  A    The first ASCII character.
  @param  B    The second ASCII character.

  @return A 16-bit value built from the two ASCII characters specified by A and B.

**/
#define SIGNATURE_16(A, B)  ((A) | (B << 8))

/**
  Returns a 32-bit signature built from 4 ASCII characters.

  This macro returns a 32-bit value built from the four ASCII characters specified
  by A, B, C, and D.

  @param  A    The first ASCII character.
  @param  B    The second ASCII character.
  @param  C    The third ASCII character.
  @param  D    The fourth ASCII character.

  @return A 32-bit value built from the two ASCII characters specified by A, B,
          C and D.

**/
#define SIGNATURE_32(A, B, C, D)  (SIGNATURE_16 (A, B) | (SIGNATURE_16 (C, D) << 16))

/**
  Returns a 64-bit signature built from 8 ASCII characters.

  This macro returns a 64-bit value built from the eight ASCII characters specified
  by A, B, C, D, E, F, G,and H.

  @param  A    The first ASCII character.
  @param  B    The second ASCII character.
  @param  C    The third ASCII character.
  @param  D    The fourth ASCII character.
  @param  E    The fifth ASCII character.
  @param  F    The sixth ASCII character.
  @param  G    The seventh ASCII character.
  @param  H    The eighth ASCII character.

  @return A 64-bit value built from the two ASCII characters specified by A, B,
          C, D, E, F, G and H.

**/
#define SIGNATURE_64(A, B, C, D, E, F, G, H) \
    (SIGNATURE_32 (A, B, C, D) | ((UINT64) (SIGNATURE_32 (E, F, G, H)) << 32))

#define EFI_PART_TYPE_UNUSED_GUID \
  { \
    0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } \
  }

#define EFI_PART_TYPE_EFI_SYSTEM_PART_GUID \
  { \
    0xc12a7328, 0xf81f, 0x11d2, {0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b } \
  }

#define EFI_PART_TYPE_LEGACY_MBR_GUID \
  { \
    0x024dee41, 0x33e7, 0x11d3, {0x9d, 0x69, 0x00, 0x08, 0xc7, 0x81, 0xf3, 0x9f } \
  }

#define EFI_PART_TYPE_MICROSOFT_BASIC_DATA_GUID \
  { \
    0xEBD0A0A2, 0xB9E5, 0x4433, { 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 } \
  }

#define EFI_PART_TYPE_LINUX_SWAP_GUID \
  { \
    0x0657FD6D, 0xA4AB, 0x43C4, { 0x84, 0xE5, 0x09, 0x33, 0xC8, 0x4B, 0x4F, 0x4F } \
  }

#define MBR_SIGNATURE  0xaa55

#define EXTENDED_DOS_PARTITION      0x05
#define EXTENDED_WINDOWS_PARTITION  0x0F

#define MAX_MBR_PARTITIONS  4

#define PMBR_GPT_PARTITION  0xEE
#define EFI_PARTITION       0xEF

#define MBR_SIZE  512

///
/// The primary GUID Partition Table Header must be
/// located in LBA 1 (i.e., the second logical block).
///
#define PRIMARY_PART_HEADER_LBA  1
///
/// EFI Partition Table Signature: "EFI PART".
///
#define EFI_PTAB_HEADER_ID  SIGNATURE_64 ('E','F','I',' ','P','A','R','T')
///
/// Minimum bytes reserve for EFI entry array buffer.
///
#define EFI_GPT_PART_ENTRY_MIN_SIZE  16384

#define EFI_GPT_REVISION 0x00010000

#pragma pack(push, 1)

///
/// MBR Partition Entry
///
typedef struct {
  UINT8    BootIndicator;
  UINT8    StartHead;
  UINT8    StartSector;
  UINT8    StartTrack;
  UINT8    OSIndicator;
  UINT8    EndHead;
  UINT8    EndSector;
  UINT8    EndTrack;
  UINT8    StartingLBA[4];
  UINT8    SizeInLBA[4];
} MBR_PARTITION_RECORD;

///
/// MBR Partition Table
///
typedef struct {
  UINT8                   BootStrapCode[440];
  UINT8                   UniqueMbrSignature[4];
  UINT8                   Unknown[2];
  MBR_PARTITION_RECORD    Partition[MAX_MBR_PARTITIONS];
  UINT16                  Signature;
} MASTER_BOOT_RECORD;

typedef UINT64 EFI_LBA;

typedef struct _EFI_GUID
{
  UINT32    Data1;
  UINT16    Data2;
  UINT16    Data3;
  UINT8     Data4[8];
} EFI_GUID;

///
/// Data structure that precedes all of the standard EFI table types.
///
typedef struct
{
  ///
  /// A 64-bit signature that identifies the type of table that follows.
  /// Unique signatures have been generated for the EFI System Table,
  /// the EFI Boot Services Table, and the EFI Runtime Services Table.
  ///
  UINT64    Signature;
  ///
  /// The revision of the EFI Specification to which this table
  /// conforms. The upper 16 bits of this field contain the major
  /// revision value, and the lower 16 bits contain the minor revision
  /// value. The minor revision values are limited to the range of 00..99.
  ///
  UINT32    Revision;
  ///
  /// The size, in bytes, of the entire table including the EFI_TABLE_HEADER.
  ///
  UINT32    HeaderSize;
  ///
  /// The 32-bit CRC for the entire table. This value is computed by
  /// setting this field to 0, and computing the 32-bit CRC for HeaderSize bytes.
  ///
  UINT32    CRC32;
  ///
  /// Reserved field that must be set to 0.
  ///
  UINT32    Reserved;
} EFI_TABLE_HEADER;

///
/// GPT Partition Table Header.
///
typedef struct
{
  ///
  /// The table header for the GPT partition Table.
  /// This header contains EFI_PTAB_HEADER_ID.
  ///
  EFI_TABLE_HEADER    Header;
  ///
  /// The LBA that contains this data structure.
  ///
  EFI_LBA             MyLBA;
  ///
  /// LBA address of the alternate GUID Partition Table Header.
  ///
  EFI_LBA             AlternateLBA;
  ///
  /// The first usable logical block that may be used
  /// by a partition described by a GUID Partition Entry.
  ///
  EFI_LBA             FirstUsableLBA;
  ///
  /// The last usable logical block that may be used
  /// by a partition described by a GUID Partition Entry.
  ///
  EFI_LBA             LastUsableLBA;
  ///
  /// GUID that can be used to uniquely identify the disk.
  ///
  EFI_GUID            DiskGUID;
  ///
  /// The starting LBA of the GUID Partition Entry array.
  ///
  EFI_LBA             PartitionEntryLBA;
  ///
  /// The number of Partition Entries in the GUID Partition Entry array.
  ///
  UINT32              NumberOfPartitionEntries;
  ///
  /// The size, in bytes, of each the GUID Partition
  /// Entry structures in the GUID Partition Entry
  /// array. This field shall be set to a value of 128 x 2^n where n is
  /// an integer greater than or equal to zero (e.g., 128, 256, 512, etc.).
  ///
  UINT32              SizeOfPartitionEntry;
  ///
  /// The CRC32 of the GUID Partition Entry array.
  /// Starts at PartitionEntryLBA and is
  /// computed over a byte length of
  /// NumberOfPartitionEntries * SizeOfPartitionEntry.
  ///
  UINT32              PartitionEntryArrayCRC32;
} EFI_PARTITION_TABLE_HEADER;

///
/// GPT Partition Entry.
///
typedef struct
{
  ///
  /// Unique ID that defines the purpose and type of this Partition. A value of
  /// zero defines that this partition entry is not being used.
  ///
  EFI_GUID    PartitionTypeGUID;
  ///
  /// GUID that is unique for every partition entry. Every partition ever
  /// created will have a unique GUID.
  /// This GUID must be assigned when the GUID Partition Entry is created.
  ///
  EFI_GUID    UniquePartitionGUID;
  ///
  /// Starting LBA of the partition defined by this entry
  ///
  EFI_LBA     StartingLBA;
  ///
  /// Ending LBA of the partition defined by this entry.
  ///
  EFI_LBA     EndingLBA;
  ///
  /// Attribute bits, all bits reserved by UEFI
  /// Bit 0:      If this bit is set, the partition is required for the platform to function. The owner/creator of the
  ///             partition indicates that deletion or modification of the contents can result in loss of platform
  ///             features or failure for the platform to boot or operate. The system cannot function normally if
  ///             this partition is removed, and it should be considered part of the hardware of the system.
  ///             Actions such as running diagnostics, system recovery, or even OS install or boot, could
  ///             potentially stop working if this partition is removed. Unless OS software or firmware
  ///             recognizes this partition, it should never be removed or modified as the UEFI firmware or
  ///             platform hardware may become non-functional.
  /// Bit 1:      If this bit is set, then firmware must not produce an EFI_BLOCK_IO_PROTOCOL device for
  ///             this partition. By not producing an EFI_BLOCK_IO_PROTOCOL partition, file system
  ///             mappings will not be created for this partition in UEFI.
  /// Bit 2:      This bit is set aside to let systems with traditional PC-AT BIOS firmware implementations
  ///             inform certain limited, special-purpose software running on these systems that a GPT
  ///             partition may be bootable. The UEFI boot manager must ignore this bit when selecting
  ///             a UEFI-compliant application, e.g., an OS loader.
  /// Bits 3-47:  Undefined and must be zero. Reserved for expansion by future versions of the UEFI
  ///             specification.
  /// Bits 48-63: Reserved for GUID specific use. The use of these bits will vary depending on the
  ///             PartitionTypeGUID. Only the owner of the PartitionTypeGUID is allowed
  ///             to modify these bits. They must be preserved if Bits 0-47 are modified..
  ///
  UINT64    Attributes;
  ///
  /// Null-terminated name of the partition.
  ///
  CHAR16    PartitionName[36];
} EFI_PARTITION_ENTRY;

#pragma pack(pop)
