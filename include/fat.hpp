#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <fstream>
#include <memory>

#include <fat_types.hpp>
#include <gpt.hpp>
#include <memory_map.hpp>

class Fat
{
    private:
        struct FatRawDirectory
        {
            DIR_ENTRY *self; // DIR_ENTRY declaring this directory
            DWORD cluster; // Current cluster
            DWORD entryIndex; // Next entry index in the cluster
        };

        struct FatDirectory
        {
            FatRawDirectory rawDirectory;
            std::unordered_map<std::string, FatDirectory> children; 
        };

        std::string destination;
        DWORD reservedSectorCount;
        DWORD sectorsPerCluster;
        DWORD clusterSize;
        DWORD numberOfFats;
        DWORD firstFsInfoSec;
        DWORD secondFsInfoSec;
        DWORD fatSize; // sectors
        std::fstream os;
        GptPartition partition;
        DWORD maxDirEntries;
        BYTE *partitionStart;
        BYTE *firstFsInfo;
        BYTE *secondFsInfo;
        DWORD *fat0;
        DWORD *fat1;
        DWORD nextFreeCluster;
        DWORD freeClusterCount;
        BYTE *dataStart;
        MemoryMappedFile file;
        FatDirectory rootDirectory;

        DWORD computeFatSizeInSectors();
        std::unique_ptr<FAT_BPB> getFatBiosParameterBlock();
        std::unique_ptr<FSINFO> getFatFsInfo();
        DWORD getFirstSectorOfCluster(DWORD cluster);
        void writeToFatEntry(DWORD fatTable, DWORD fatEntry, DWORD value);
        void writeToSector(DWORD sector, BYTE* buffer, DWORD size);
        void createRootDirectory();
        std::pair<FATDATE, FATTIME> getCurrentDateAndTime();
        std::unique_ptr<BYTE[]> getDirectoryEntry(std::string const& name, bool directory, DWORD firstCluster, DWORD size, std::optional<std::pair<FATDATE, FATTIME>> const& dateTime, DWORD &bufferSize);
        DWORD allocateClusters(DWORD previousCluster, DWORD clusterCount);
        BYTE *getPointerToCluster(DWORD cluster);
        std::pair<DWORD, DWORD> writeFile(std::string const& sourcePath);
        bool writeDirectoryEntries(FatRawDirectory &directory, std::unique_ptr<BYTE[]> &entryBuffer, DWORD entryBufferSize);
        bool createRawFile(FatRawDirectory &directory, std::string const& filename, std::string const& sourcePath);
        std::optional<FatRawDirectory> createRawDirectory(FatRawDirectory &parent, std::string const& directoryName);
        FatDirectory* findDirectory(std::string const& path);

    public:
        Fat(std::string const &outputPath, GptPartition const &partition);

        void createFilesystem();
        void openFilesystem();
        void closeFilesystem();
        bool createDirectory(std::string const& path);
        bool createFile(std::string const& destinationPath, std::string const& sourcePath);
};

