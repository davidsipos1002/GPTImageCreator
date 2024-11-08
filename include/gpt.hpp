#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <fstream>
#include <memory>

#include <gpt_types.hpp>

#define SECTOR_SIZE 512

typedef struct
{
    EFI_GUID Type; 
    QWORD LBACount;
    std::u16string PartitionName;
} ConfigurationParitition;

typedef struct
{
    EFI_GUID Type;
    EFI_GUID PartitionId;
    QWORD LBACount;
    QWORD StartingLBA;
    QWORD EndingLBA;
    std::u16string PartitionName;
} GptPartition;

class GptDisk
{
    private:
        std::vector<GptPartition> gptPartitions;
        std::fstream os;
        QWORD diskSize;
        EFI_GUID diskId;
        EFI_LBA primaryHeader; 
        EFI_LBA partitionTable;
        EFI_LBA firstUsable;
        EFI_LBA lastUsable;
        EFI_LBA backupPartitionTable;
        EFI_LBA secondaryHeader;
        EFI_LBA last;
        DWORD gptHeaderSize;
        DWORD partitionEntrySize;
        bool diskCreated;

        EFI_GUID generateUuid();
        std::unique_ptr<MASTER_BOOT_RECORD> getGptProtectiveMbr();
        std::unique_ptr<EFI_PARTITION_ENTRY> getEfiPartitionEntry(GptPartition const& partition);
        std::unique_ptr<EFI_PARTITION_TABLE_HEADER> getInitialEfiPartitionTableHeader();
        BYTE* generatePartitionTable();

    public:
        GptDisk(std::string const& outputPath);

        void configureDisk(std::vector<ConfigurationParitition> const& config);
        void createDisk();
        std::optional<GptPartition> getPartition(std::u16string const& partitionName);
        std::optional<QWORD> getDiskSize();

};
