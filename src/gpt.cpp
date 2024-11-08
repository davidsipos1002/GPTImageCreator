#include <gpt.hpp>

#include <cstring>

#include <crc32.hpp>
#include <guid.hpp>

GptDisk::GptDisk(std::string const& outputPath) : os(outputPath, std::ios::out | std::ios::in | std::ios::binary) {}

EFI_GUID GptDisk::generateUuid()
{
    xg::Guid guid = xg::newGuid();
    std::array<UINT8, 16> byteArray = guid.bytes();

    EFI_GUID uuid;
    uuid.Data1 = (byteArray[0] << 24) | (byteArray[1] << 16) | (byteArray[2] << 8) | byteArray[3];
    uuid.Data2 = (byteArray[4] << 8) | byteArray[5];
    uuid.Data3 = (byteArray[6] << 8) | byteArray[7];
    uuid.Data4[0] = byteArray[8];
    uuid.Data4[1] = byteArray[9];
    uuid.Data4[2] = byteArray[10];
    uuid.Data4[3] = byteArray[11];
    uuid.Data4[4] = byteArray[12];
    uuid.Data4[5] = byteArray[13];
    uuid.Data4[6] = byteArray[14];
    uuid.Data4[7] = byteArray[15];

    return uuid;
}

void GptDisk::configureDisk(std::vector<ConfigurationParitition> const& config)
{
    // Our GPT Disk Layout
    // LBA 0: Protective MBR
    // LBA 1: Primary GPT Header
    // LBA 2: Primary Partition Table
    // LBA 3: Padding
    // LBA 4: FirstUsable
    // LBA FirstStart
    // ...
    // LBA FirstLast
    // LBA Padding 1 LBA
    // LBA Padding 1 LBA
    // LBA Secondary Paritition Table
    // ...
    // LBA Secondary GPT Header

    // Generate GPT Disk ID
    diskId = generateUuid();

    // Set the sizes
    gptHeaderSize = SECTOR_SIZE;
    partitionEntrySize = 2 * EFI_GPT_PART_ENTRY_MIN_SIZE;

    // Compute Header LBAs
    primaryHeader = 1;
    partitionTable = 2;
    firstUsable = partitionTable + (partitionEntrySize * config.size()) / 512 + 1;

    // Compute all LBAs
    GptPartition gptPartition;
    EFI_LBA currentLba = firstUsable;
    for (auto const& part : config)
    {
        gptPartition.Type = part.Type;
        gptPartition.PartitionId = generateUuid();
        gptPartition.LBACount = part.LBACount;
        gptPartition.StartingLBA = currentLba;
        currentLba += gptPartition.LBACount - 1;
        gptPartition.EndingLBA = currentLba;
        gptPartition.PartitionName = part.PartitionName;

        // Add the padding
        currentLba += 2;
        gptPartitions.push_back(gptPartition);
    }

    lastUsable = currentLba;
    backupPartitionTable = lastUsable + 1;
    secondaryHeader = backupPartitionTable + (partitionEntrySize * config.size()) / 512;
    last = secondaryHeader + 2; 

    diskSize = (last - 1) * SECTOR_SIZE;

    os.seekp(0);
}

std::unique_ptr<MASTER_BOOT_RECORD> GptDisk::getGptProtectiveMbr()
{
    auto ret = std::make_unique<MASTER_BOOT_RECORD>();
    
    ret->Partition[0].StartSector = 0x02;
    ret->Partition[0].OSIndicator = PMBR_GPT_PARTITION;
    ret->Partition[0].EndHead = 0xFF;
    ret->Partition[0].EndSector = 0xFF;
    ret->Partition[0].EndTrack = 0xFF;
    ret->Partition[0].StartingLBA[0] = 1;
    ret->Partition[0].SizeInLBA[0] = 0xFF;
    ret->Partition[0].SizeInLBA[1] = 0xFF;
    ret->Partition[0].SizeInLBA[2] = 0xFF;
    ret->Partition[0].SizeInLBA[3] = 0xFF;
    ret->Signature = 0xAA55;

    return ret;
}

std::unique_ptr<EFI_PARTITION_ENTRY> GptDisk::getEfiPartitionEntry(GptPartition const& partition)
{
    auto ret = std::make_unique<EFI_PARTITION_ENTRY>();

    ret->PartitionTypeGUID = partition.Type;
    ret->UniquePartitionGUID = partition.PartitionId;
    ret->StartingLBA = partition.StartingLBA;
    ret->EndingLBA = partition.EndingLBA;
    ret->Attributes = 1;
    std::memcpy(ret->PartitionName,
                partition.PartitionName.c_str(), 
                sizeof(CHAR16) * std::min(static_cast<size_t>(32), partition.PartitionName.size()));

    return ret;
}

BYTE* GptDisk::generatePartitionTable()
{
    QWORD bufferLength = partitionEntrySize * gptPartitions.size();
    BYTE *ret = new BYTE[bufferLength];
    
    std::memset(ret, 0, bufferLength);

    QWORD ndx = 0;
    for (auto const& part : gptPartitions)
    {
        auto tableEntry = getEfiPartitionEntry(part);
        std::memcpy(ret + ndx, tableEntry.get(), sizeof(EFI_PARTITION_ENTRY));
        ndx += partitionEntrySize;
    }

    return ret;
}

std::unique_ptr<EFI_PARTITION_TABLE_HEADER> GptDisk::getInitialEfiPartitionTableHeader()
{
    auto ret = std::make_unique<EFI_PARTITION_TABLE_HEADER>();

    ret->Header.Signature = EFI_PTAB_HEADER_ID;
    ret->Header.Revision = EFI_GPT_REVISION;
    ret->Header.HeaderSize = gptHeaderSize; 
    ret->MyLBA = primaryHeader;
    ret->AlternateLBA = secondaryHeader;
    ret->FirstUsableLBA = firstUsable;
    ret->LastUsableLBA = lastUsable;
    ret->DiskGUID = diskId;
    ret->PartitionEntryLBA = partitionTable;
    ret->NumberOfPartitionEntries = gptPartitions.size();
    ret->SizeOfPartitionEntry = partitionEntrySize;

    return ret;
}

void GptDisk::createDisk()
{
    // Write the Protective MBR
    auto mbr = getGptProtectiveMbr();
    os.write(reinterpret_cast<const char*>(mbr.get()), sizeof(MASTER_BOOT_RECORD));

    // Generate the partition table first, to compute its CRC32 for the header
    BYTE *gptPartitionTable = generatePartitionTable();
    QWORD gptPartitionTableLength = partitionEntrySize * gptPartitions.size();

    // Compute partition table CRC32
    DWORD partitionCrc32 = computeCrc32(gptPartitionTable, gptPartitionTableLength);

    UINT8 *headerBuffer = new UINT8[SECTOR_SIZE];
    std::memset(headerBuffer, 0, SECTOR_SIZE);

    auto gptHeader = getInitialEfiPartitionTableHeader();
    gptHeader->PartitionEntryArrayCRC32 = partitionCrc32;
    std::memcpy(headerBuffer, gptHeader.get(), sizeof(EFI_PARTITION_TABLE_HEADER));

    // Compute GPT Header CRC32
    DWORD gptHeaderCrc32 = computeCrc32(headerBuffer, SECTOR_SIZE);
    gptHeader->Header.CRC32 = gptHeaderCrc32;

    // Write Primary GPT Header
    os.write(reinterpret_cast<const char*>(gptHeader.get()), sizeof(EFI_PARTITION_TABLE_HEADER));
    os.seekp(partitionTable * SECTOR_SIZE);

    // Write Primary Partition Table
    os.write(reinterpret_cast<const char*>(gptPartitionTable), gptPartitionTableLength);

    // Jump to and write Secondary Partition Table
    os.seekp(backupPartitionTable * SECTOR_SIZE);
    os.write(reinterpret_cast<const char*>(gptPartitionTable), gptPartitionTableLength);

    // Prepare Secondary GPT Header
    gptHeader->MyLBA = secondaryHeader;
    gptHeader->AlternateLBA = primaryHeader;
    gptHeader->Header.CRC32 = 0;
    std::memcpy(headerBuffer, gptHeader.get(), sizeof(EFI_PARTITION_TABLE_HEADER));
    gptHeaderCrc32 = computeCrc32(headerBuffer, SECTOR_SIZE);
    os.write(reinterpret_cast<const char*>(gptHeader.get()), sizeof(EFI_PARTITION_TABLE_HEADER));

    delete[] headerBuffer;
    delete[] gptPartitionTable;

    diskCreated = true;
    os.close();
}

std::optional<GptPartition> GptDisk::getPartition(std::u16string const& partitionName)
{
    if (!diskCreated)
        return {};
    
    for (auto const& part : gptPartitions)
    {
        if (part.PartitionName == partitionName)
            return part;
    }

    return {};
}

std::optional<QWORD> GptDisk::getDiskSize()
{
    if (!diskCreated)
        return {};
    return diskSize;
}
