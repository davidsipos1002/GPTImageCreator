#pragma once

#include <cal_types.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////                                    DEFINES                                    /////////
/////////////////////////////////////////////////////////////////////////////////////////////////

// FAT Directory Entry Attributes
#define        ATTR_NORMAL                  0x00
#define        ATTR_READ_ONLY               0x01
#define        ATTR_HIDDEN                  0x02
#define        ATTR_SYSTEM                  0x04
#define        ATTR_VOLUME_ID               0x08
#define        ATTR_DIRECTORY               0x10
#define        ATTR_ARCHIVE                 0x20

#define        ATTR_LONG_NAME               (ATTR_READ_ONLY|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME_ID)
#define        ATTR_LONG_NAME_MASK          (ATTR_READ_ONLY|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME_ID|ATTR_DIRECTORY|ATTR_ARCHIVE)

// Short Directory entry name length
#define        SHORT_NAME_NAME              8
#define        SHORT_NAME_EXT               3
#define        SHORT_NAME_CHARS             11

// Long Directory Entries char lengths
#define        LONG_NAME1_CHARS             5
#define        LONG_NAME2_CHARS             6
#define        LONG_NAME3_CHARS             2
#define        LONG_NAME_TOTAL_CHARS        (LONG_NAME1_CHARS+LONG_NAME2_CHARS+LONG_NAME3_CHARS)

#define        LONG_NAME_MAX_CHARS          255

#define        MAX_SECTORS_FOR_LONG_ENTRY    3

// Values in DIR_Name[0] to check to see if entry
// is a valid directory entry
#define        FREE_ENTRY                   0xE5
#define        FREE_ALL                     0x00
#define        FREE_JAP_ENTRY               0x05

#define        LONG_NAME_ORD_END_MASK       0x40

// FAT Date format values
#define        FAT_DAY_BITS                 5
#define        FAT_MONTH_BITS               4
#define        FAT_YEAR_BITS                7

#define        FAT_DAY_RANGE_MIN            1
#define        FAT_DAY_RANGE_MAX            31

#define        FAT_MONTH_RANGE_MIN          1
#define        FAT_MONTH_RANGE_MAX          12

#define        FAT_START_YEAR               1980

// FAT Time format values
#define        FAT_SECOND_BITS              5
#define        FAT_MINUTE_BITS              6
#define        FAT_HOUR_BITS                5

#define        FAT_SECOND_RANGE_MIN         0
#define        FAT_SECOND_RANGE_MAX         29

#define        FAT_MINUTE_RANGE_MIN         0
#define        FAT_MINUTE_RANGE_MAX         59

#define        FAT_HOUR_RANGE_MIN           0
#define        FAT_HOUR_RANGE_MAX           23

// Path delimiter
#define        FAT_DELIMITER                '\\'

// Refers to dot and dotdot entries
#define        FAT_DIR_NO_DEFAULT_ENTRIES   2

// Refers to the index in the DIR_ENTRY array
// of the dot and dotdot entries
#define        DOT_ENTRY_INDEX              0
#define        DOT_DOT_ENTRY_INDEX          1

// Dot and dotdot entries
#define        DOT_ENTRY_NAME               ".          "
#define        DOT_DOT_ENTRY_NAME           "..         "

// Mask to apply when reading cluster values
#define        FAT32_CLUSTER_MASK           0x0FFFFFFF

// End of clusterchain mark
#define        FAT32_EOC_MARK               0x0FFFFFFF

// Bad cluster value
#define        FAT32_BAD_CLUSTER            0x0FFFFFF7

#define        FAT32_FAT_RESERVED           0x0FFFFFF8

// Checks if cluster number marks the end of the cluster chain
#define        FAT32_EOC(cluster)           ((cluster) > FAT32_BAD_CLUSTER)

#define        FAT32_UNKNOWN                0xFFFFFFFF

// Maximum number of clusters per FAT type
#define        FAT12_MAX_CLUSTERS           4085
#define        FAT16_MAX_CLUSTERS           65525

//typedef     WORD    FAT16_ENTRY;
typedef     DWORD   FAT32_ENTRY;

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////                                    STRUCTURES                                 /////////
/////////////////////////////////////////////////////////////////////////////////////////////////


// Directory entry structure
#pragma pack(push,1)

typedef struct _FATDATE
{
    // 1 -> 31
    WORD        Day     : 5;
    WORD        Month   : 4;

    // this value must be incremented
    // by 1980
    WORD        Year    : 7;
} FATDATE, *PFATDATE;

typedef struct _FATTIME
{
    // Multiply by 2 to get second count
    WORD        Second  : 5;
    WORD        Minute  : 6;
    WORD        Hour    : 5;
} FATTIME, *PFATTIME;

typedef struct _DIR_ENTRY
{
    BYTE            DIR_Name[11];       // 0x0  Name
    BYTE            DIR_Attr;           // 0xB  Attributes
    BYTE            DIR_NTRes;          // 0xC  Reserved

    // Creation Time and Date
    BYTE            DIR_CrtTimeTenth;   // 0xD
    FATTIME         DIR_CrtTime;        // 0xE
    FATDATE         DIR_CrtDate;        // 0x10

    // Last Access Date
    FATDATE         DIR_LstAccDate;     // 0x12

    // Directory Entry Cluster High
    WORD            DIR_FstClusHI;      // 0x14

    // Write Time and Date
    FATTIME         DIR_WrtTime;        // 0x16
    FATDATE         DIR_WrtDate;        // 0x18

    // Directory Entry Cluster Low
    WORD            DIR_FstClusLO;      // 0x1A

    // File Size( 0 for directories )
    DWORD           DIR_FileSize;       // 0x1C
} DIR_ENTRY, *PDIR_ENTRY;

// Long directory entry structure
typedef struct _LONG_DIR_ENTRY
{
    // If Handled properly indicates the index of this entry in a long entry list
    BYTE            LDIR_Ord;

    // Characters 1-5 of name
    BYTE            LDIR_Name1[10];

    // File attributes
    BYTE            LDIR_Attr;

    BYTE            LDIR_Type;
    BYTE            LDIR_Chksum;

    // Characters 6-11 of name
    BYTE            LDIR_Name2[12];
    WORD            LDIR_FstClusLO;     // Must be 0 ( ZERO )

                                        // Characters 12-13 of name
    BYTE            LDIR_Name3[4];
} LONG_DIR_ENTRY, *PLONG_DIR_ENTRY;

// Found in sector 1 of the partition
typedef struct _FSINFO
{
    DWORD           FSI_LeadSig;
    BYTE            FSI_Reserved1[480];
    DWORD           FSI_StrucSig;
    DWORD           FSI_Free_Count;      // number of free clusters remaining
    DWORD           FSI_Nxt_Free;        // next free cluster
    BYTE            FSI_Reserved2[12];
    DWORD           FSI_TrailSig;
} FSINFO, *PFSINFO;

// Found in sector 0 of the partition
typedef struct _FAT_BPB
{
    BYTE            BS_jmpBoot[3];      // jump instruction
    char            BS_OEMName[8];
    WORD            BPB_BytsPerSec;     // bytes per sector
    BYTE            BPB_SecPerClus;     // number of sectors / cluster
    WORD            BPB_RsvdSecCnt;     // number of reserved sectors
    BYTE            BPB_NumFATs;        // number of FATS
    WORD            BPB_RootEntCnt;     // 0 for FAT32
    WORD            BPB_TotSec16;       // 0 for FAT32
    BYTE            BPB_Media;
    WORD            BPB_FATSz16;        // 0 for FAT32
    WORD            BPB_SecPerTrk;
    WORD            BPB_NumHeads;
    DWORD           BPB_HiddSec;
    DWORD           BPB_TotSec32;       // Number of sectors on volume
    union {
        struct
        {
            BYTE    BS_DrvNum;
            BYTE    BS_Reserved1;
            BYTE    BS_BootSig;
            DWORD   BS_VolID;
            BYTE    BS_VolLab[11];
            BYTE    BS_FilSysType[8];
        } FAT12_16_BPB;

        struct
        {
            DWORD   BPB_FATSz32;        // Number of Sectors occupied by a FAT32
            WORD    BPB_ExtFlags;
            WORD    BPB_FSVer;
            DWORD   BPB_RootClus;
            WORD    BPB_FSInfo;
            WORD    BPB_BkBootSec;
            BYTE    BPB_Reserved[12];
            BYTE    BS_DrvNum;
            BYTE    BS_Reserved1;
            BYTE    BS_BootSig;
            DWORD   BS_VolID;
            BYTE    BS_VolLab[11];
            BYTE    BS_FilSysType[8];   // should be set to "FAT32   " else it's bad :(
        } FAT32_BPB;
    } DiffOffset;
    BYTE            Reserved[420];
    WORD            Signature;
} FAT_BPB, *PFAT_BPB;

#pragma pack(pop)
