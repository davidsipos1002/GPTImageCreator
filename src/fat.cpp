#include <fat.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <cstring>
#include <ctime>

struct DSKSZTOSECPERCLUS
{
    // In sectors
    DWORD DiskSize;
    BYTE SecPerClusVal;
};

/*
* This is the table for FAT32 drives. NOTE that this table includes
* entries for disk sizes smaller than 512 MB even though typically
* only the entries for disks >= 512 MB in size are used.
* The way this table is accessed is to look for the first entry
* in the table for which the disk size is less than or equal
* to the DiskSize field in that table entry. For this table to
* work properly BPB_RsvdSecCnt must be 32, and BPB_NumFATs
* must be 2. Any of these values being different may require the first
* table entries DiskSize value to be changed otherwise the cluster count
* may be to low for FAT32.
*/
DSKSZTOSECPERCLUS DskTableFAT32[] = {
    {66600, 0},      /* disks up to 32.5 MB, the 0 value for SecPerClusVal trips an error */
    {532480, 1},     /* disks up to 260 MB, .5k cluster */
    {16777216, 8},   /* disks up to 8 GB, 4k cluster */
    {33554432, 16},  /* disks up to 16 GB, 8k cluster */
    {67108864, 32},  /* disks up to 32 GB, 16k cluster */
    {0xFFFFFFFF, 64} /* disks greater than 32GB, 32k cluster */
};

Fat::Fat(std::string const &outputPath, GptPartition const &partition) : destination(outputPath), os(outputPath, std::ios::out | std::ios::in | std::ios::binary), partition(partition) {}

DWORD Fat::computeFatSizeInSectors()
{
    DWORD tmpVal1 = static_cast<DWORD>(partition.LBACount) - reservedSectorCount;
    DWORD tmpVal2 = ((256 * sectorsPerCluster) + numberOfFats) / 2;
    float sz = (tmpVal1 + (tmpVal2 - 1)) / ((float) tmpVal2);
    return std::ceil(sz);
}

std::unique_ptr<FAT_BPB> Fat::getFatBiosParameterBlock()
{
    auto ret = std::make_unique<FAT_BPB>();
    
    ret->BS_jmpBoot[0] = 0xEB;
    ret->BS_jmpBoot[1] = 0xFE;
    ret->BS_jmpBoot[2] = 0x90;
    std::memcpy(ret->BS_OEMName, "SIPOSDV", 8);
    ret->BPB_BytsPerSec = SECTOR_SIZE;

    sectorsPerCluster = 0;
    for (int i = 0; i < 6; i++)
    {
        if (static_cast<DWORD>(partition.LBACount) <= DskTableFAT32[i].DiskSize) {
            sectorsPerCluster = DskTableFAT32[i].SecPerClusVal;
            break;
        }
    }
    if (!sectorsPerCluster)
        return nullptr;
    
    ret->BPB_SecPerClus = sectorsPerCluster;
    ret->BPB_RsvdSecCnt = reservedSectorCount;
    ret->BPB_NumFATs = numberOfFats;
    ret->BPB_RootEntCnt = 0;
    ret->BPB_TotSec16 = 0;
    ret->BPB_Media = 0xF8;
    ret->BPB_FATSz16 = 0;
    ret->BPB_SecPerTrk = 63;
    ret->BPB_NumHeads = 255;
    ret->BPB_HiddSec = 0;
    ret->BPB_TotSec32 = static_cast<DWORD>(partition.LBACount);

    fatSize = computeFatSizeInSectors();
    ret->DiffOffset.FAT32_BPB.BPB_FATSz32 = fatSize;
    ret->DiffOffset.FAT32_BPB.BPB_ExtFlags = 0;
    ret->DiffOffset.FAT32_BPB.BPB_FSVer = 0;
    ret->DiffOffset.FAT32_BPB.BPB_RootClus = 2;
    ret->DiffOffset.FAT32_BPB.BPB_FSInfo = 1;
    ret->DiffOffset.FAT32_BPB.BPB_BkBootSec = 6;
    ret->DiffOffset.FAT32_BPB.BS_DrvNum = 0x80;
    ret->DiffOffset.FAT32_BPB.BS_BootSig = 0x29;
    ret->DiffOffset.FAT32_BPB.BS_VolID = 0xFEEDFACE;
    std::memcpy(ret->DiffOffset.FAT32_BPB.BS_VolLab, "NO NAME   ", 11);
    std::memcpy(ret->DiffOffset.FAT32_BPB.BS_FilSysType, "FAT32  ", 8);
    ret->Signature = 0xAA55;

    return ret;
}

std::unique_ptr<FSINFO> Fat::getFatFsInfo()
{
    auto ret = std::make_unique<FSINFO>();

    ret->FSI_LeadSig = 0x41615252;
    ret->FSI_StrucSig = 0x61417272;
    ret->FSI_Free_Count = 0xFFFFFFFF;
    ret->FSI_Nxt_Free = 0xFFFFFFFF;
    ret->FSI_TrailSig = 0xAA550000;

    return ret;
}

DWORD Fat::getFirstSectorOfCluster(DWORD cluster)
{
    DWORD firstDataSector = reservedSectorCount + (numberOfFats * fatSize);
    return ((cluster - 2) * sectorsPerCluster) + firstDataSector;
}

void Fat::writeToFatEntry(DWORD fatTable, DWORD fatEntry, DWORD value)
{
    os.seekp(partition.StartingLBA * SECTOR_SIZE + reservedSectorCount * SECTOR_SIZE + (fatTable * fatSize) * SECTOR_SIZE + 4 * fatEntry);
    os.write(reinterpret_cast<const char *>(&value), sizeof(DWORD));
}

void Fat::writeToSector(DWORD sector, BYTE* buffer, DWORD size)
{
    os.seekp(partition.StartingLBA * SECTOR_SIZE + sector * SECTOR_SIZE);
    os.write(reinterpret_cast<const char *>(buffer), size);
}

void Fat::createRootDirectory()
{
    // Allocate cluster 2 to root directory
    writeToFatEntry(0, 0, 0x0FFFFFF8);
    writeToFatEntry(0, 1, FAT32_EOC_MARK);
    writeToFatEntry(0, 2, FAT32_EOC_MARK);
    writeToFatEntry(1, 0, 0x0FFFFFF8);
    writeToFatEntry(1, 1, FAT32_EOC_MARK);
    writeToFatEntry(1, 2, FAT32_EOC_MARK);
}

void Fat::createFilesystem()
{
    reservedSectorCount = 32;
    numberOfFats = 2;

    auto bpb = getFatBiosParameterBlock();
    auto fs = getFatFsInfo();

    os.seekp(partition.StartingLBA * SECTOR_SIZE);

    // Write primary headers
    os.write(reinterpret_cast<const char *>(bpb.get()), sizeof(FAT_BPB)); // Sector 0
    os.write(reinterpret_cast<const char *>(fs.get()), sizeof(FSINFO)); // Sector 1

    // Write secondary headers
    os.seekp(partition.StartingLBA * SECTOR_SIZE + 7 * SECTOR_SIZE);
    bpb.get()->DiffOffset.FAT32_BPB.BPB_FSInfo = 7;
    os.write(reinterpret_cast<const char *>(bpb.get()), sizeof(FAT_BPB)); // Sector 6
    os.write(reinterpret_cast<const char *>(fs.get()), sizeof(FSINFO)); // Sector 7

    // Zero out the whole file
    os.seekp(partition.StartingLBA * SECTOR_SIZE + partition.LBACount * SECTOR_SIZE - 1);
    char zero = 0;
    os.write(&zero, 1);

    // Create the root directory
    createRootDirectory();

    os.close();
}

std::pair<FATDATE, FATTIME> Fat::getCurrentDateAndTime()
{
    std::time_t t = std::time(NULL);
    std::tm *localTime = std::localtime(&t);

    // Set date
    FATDATE date;
    date.Day= localTime->tm_mday;
    date.Month = localTime->tm_mon + 1;
    date.Year = localTime->tm_year - 80;

    // Set time
    FATTIME time;
    int secs = localTime->tm_sec / 2;
    secs = secs > 29 ? 29 : secs;
    time.Second = secs;
    time.Minute = localTime->tm_min;
    time.Hour = localTime->tm_hour;

    return std::make_pair(date, time);
}

static bool getShortName(std::string const& name, std::string &shortName)
{
    size_t length = name.length();
    size_t dotindx = name.find('.');
    
    size_t dotlength;
    size_t namelen;
    bool isLongName = false;
    if (dotindx != std::string::npos)
    {
        if (dotindx > 8 || length - dotindx - 1 > 3)
            isLongName = true;
        dotlength = std::min(length - dotindx - 1, static_cast<size_t>(3));
        namelen = std::min(dotindx, static_cast<size_t>(8));
    }
    else
    {
        if (length > 8)
            isLongName = true;
        dotlength = 0;
        namelen = std::min(length, static_cast<size_t>(8));
    }
    
    std::string temp = "           ";
    if(dotlength)
    {
        int i = dotindx + 1;
        int j = 8;
        while(dotlength)
        {
            temp[j] = towupper(name[i]);
            i++;
            j++;
            dotlength--;
        }
    }
    
    if (namelen)
    {
        int i = 0;
        while (namelen)
        {
            temp[i] = towupper(name[i]);
            i++;
            namelen--;
        }
        if (isLongName)
        {
            temp[6] = '~';
            temp[7] = '1';
        }
    }
    
    shortName = temp;
    return isLongName;
}

static BYTE getShortNameChecksum(BYTE const *shortName)
{
    SHORT nameLen;
    BYTE sum = 0;
    for (nameLen=11; nameLen!=0; nameLen--) {
        // NOTE: The operation is an unsigned char rotate right
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *shortName++;
    }
    return sum;
}

std::unique_ptr<BYTE[]> Fat::getDirectoryEntry(std::string const& name, bool directory, DWORD firstCluster, DWORD size, std::optional<std::pair<FATDATE, FATTIME>> const& dateTime, DWORD &bufferSize)
{
    bufferSize = 0;
    if (name.length() > 255)
        return nullptr;
    
    std::string shortName;
    bool isLongName = getShortName(name, shortName);
    DWORD longNameEntryCount = isLongName ? (name.length() / 13 + (name.length() % 13 ? 1 : 0)) : 0;
    bufferSize = longNameEntryCount * sizeof(LONG_DIR_ENTRY) + sizeof(DIR_ENTRY);

    auto buff = std::unique_ptr<BYTE[]>(new BYTE[bufferSize]);
    std::memset(buff.get(), 0, bufferSize);

    if (isLongName)
    {
        BYTE checkSum = getShortNameChecksum(reinterpret_cast<BYTE const*>(shortName.c_str()));

        // Generate last entry
        LONG_DIR_ENTRY *ll = (LONG_DIR_ENTRY *) buff.get();
        std::memset(ll, 0xFF, sizeof(LONG_DIR_ENTRY));
        ll->LDIR_Ord = LONG_NAME_ORD_END_MASK | static_cast<BYTE>(longNameEntryCount);
        ll->LDIR_Attr = ATTR_LONG_NAME;
        ll->LDIR_Type = 0;
        ll->LDIR_Chksum = checkSum;
        ll->LDIR_FstClusLO = 0;
        DWORD startIndx = (longNameEntryCount - 1) * 13;
        DWORD lastCharCount = name.length() - startIndx;
        if (lastCharCount < 13)
        {
            if (lastCharCount < 5)
            {
                ll->LDIR_Name1[2 * lastCharCount] = 0;
                ll->LDIR_Name1[2 * lastCharCount + 1] = 0;
            }
            else if (lastCharCount < 11)
            {
                ll->LDIR_Name2[2 * (lastCharCount - 5)] = 0;
                ll->LDIR_Name2[2 * (lastCharCount - 5) + 1] = 0;
            }
            else
            {
                ll->LDIR_Name3[2 * (lastCharCount - 11)] = 0;
                ll->LDIR_Name3[2 * (lastCharCount - 11) + 1] = 0;
            }
        }
        DWORD entryIndex = 0;
        for (DWORD i = startIndx; i < startIndx + lastCharCount; i++)
        {
            if (entryIndex < 5)
            {
                ll->LDIR_Name1[2 * entryIndex] = name[i];
                ll->LDIR_Name1[2 * entryIndex + 1] = 0;
            }
            else if (entryIndex < 11)
            {
                ll->LDIR_Name2[2 * (entryIndex - 5)] = name[i];
                ll->LDIR_Name2[2 * (entryIndex - 5) + 1] = 0;
            }
            else
            {
                ll->LDIR_Name3[2 * (entryIndex - 11)] = name[i];
                ll->LDIR_Name3[2 * (entryIndex - 11) + 1] = 0;
            }
            entryIndex++;
        }

        for (DWORD i = 1; i < longNameEntryCount; i++)
        {
            ll = (LONG_DIR_ENTRY *) (buff.get() + i * sizeof(LONG_DIR_ENTRY));
            ll->LDIR_Ord = longNameEntryCount - i;
            ll->LDIR_Attr = ATTR_LONG_NAME;
            ll->LDIR_Type = 0;
            ll->LDIR_Chksum = checkSum;
            ll->LDIR_FstClusLO = 0; 

            startIndx = (longNameEntryCount - i - 1) * 13;
            entryIndex = 0;
            for (DWORD j = startIndx; j < startIndx + 13; j++)
            {
                if (entryIndex < 5)
                {
                    ll->LDIR_Name1[2 * entryIndex] = name[j];
                    ll->LDIR_Name1[2 * entryIndex + 1] = 0;
                }
                else if (entryIndex < 11)
                {
                    ll->LDIR_Name2[2 * (entryIndex - 5)] = name[j];
                    ll->LDIR_Name2[2 * (entryIndex - 5) + 1] = 0;
                }
                else
                {
                    ll->LDIR_Name3[2 * (entryIndex - 11)] = name[j];
                    ll->LDIR_Name3[2 * (entryIndex - 11) + 1] = 0;
                }
                entryIndex++;
            }
        } 
    }

    std::pair<FATDATE, FATTIME> dateTimePair;
    if (dateTime.has_value())
        dateTimePair = dateTime.value();
    else
        dateTimePair = getCurrentDateAndTime();

    DIR_ENTRY *dirEntry = (DIR_ENTRY *) (buff.get() + longNameEntryCount * sizeof(DIR_ENTRY)); 
    for (int i = 0; i < 11; i++)
        dirEntry->DIR_Name[i] = shortName[i];
    dirEntry->DIR_Attr = directory ? ATTR_DIRECTORY : ATTR_ARCHIVE;
    dirEntry->DIR_CrtTimeTenth = 0;
    dirEntry->DIR_CrtTime = dateTimePair.second;
    dirEntry->DIR_CrtDate = dateTimePair.first;
    dirEntry->DIR_LstAccDate = dateTimePair.first;
    dirEntry->DIR_FstClusHI = firstCluster >> 16;
    dirEntry->DIR_WrtTime = dateTimePair.second;
    dirEntry->DIR_WrtDate = dateTimePair.first;
    dirEntry->DIR_FstClusLO = firstCluster & UINT16_MAX;
    dirEntry->DIR_FileSize = directory ? 0 : size;

    return buff;
}

void Fat::openFilesystem()
{
    assert(sizeof(DIR_ENTRY) == sizeof(LONG_DIR_ENTRY));

    BYTE *ptr = static_cast<BYTE *>(openMemoryMappedFile(&file, destination.c_str()));

    partitionStart = ptr + partition.StartingLBA * SECTOR_SIZE;

    firstFsInfo = partitionStart + firstFsInfoSec * SECTOR_SIZE;
    secondFsInfo = partitionStart + secondFsInfoSec * SECTOR_SIZE;

    fat0 = reinterpret_cast<DWORD *>(partitionStart + reservedSectorCount * SECTOR_SIZE);
    fat1 = reinterpret_cast<DWORD *>(partitionStart + reservedSectorCount * SECTOR_SIZE + fatSize * SECTOR_SIZE);
    
    nextFreeCluster = 3;
    DWORD dataSectors = partition.LBACount - (reservedSectorCount + numberOfFats * fatSize);
    freeClusterCount = dataSectors / sectorsPerCluster - 1; // 1 cluster reserved for the root directory
    clusterSize = sectorsPerCluster * SECTOR_SIZE;
    maxDirEntries = clusterSize / sizeof(DIR_ENTRY);

    dataStart = partitionStart + reservedSectorCount * SECTOR_SIZE + numberOfFats * fatSize * SECTOR_SIZE;

    rootDirectory.rawDirectory.self = NULL;
    rootDirectory.rawDirectory.cluster = 2;
    rootDirectory.rawDirectory.entryIndex = 0;
}

void Fat::closeFilesystem()
{
    // We write the fs info information because we now know eveything
    // because we created every file and directory

    FSINFO *fsInfo = reinterpret_cast<FSINFO*>(firstFsInfo);
    // Update first fs info
    fsInfo->FSI_Free_Count = freeClusterCount;
    fsInfo->FSI_Nxt_Free = nextFreeCluster;
    // Update second fs info
    fsInfo = reinterpret_cast<FSINFO*>(secondFsInfo);
    fsInfo->FSI_Free_Count = freeClusterCount;
    fsInfo->FSI_Nxt_Free = nextFreeCluster;
    
    // Now we can close the file
    closeMemoryMappedFile(&file);
}

DWORD Fat::allocateClusters(DWORD previousCluster, DWORD clusterCount)
{
    if (clusterCount > freeClusterCount)
        return UINT32_MAX;

    freeClusterCount -= clusterCount;

    DWORD ret = nextFreeCluster;
    if (previousCluster != UINT32_MAX)
    {
        fat0[previousCluster] = nextFreeCluster;
        fat1[previousCluster] = nextFreeCluster;
    }

    DWORD currentCluster = nextFreeCluster;
    for (int i = 0; i < clusterCount; i++)
    {
        if (i != clusterCount - 1)
        {
            fat0[currentCluster] = currentCluster + 1;
            fat1[currentCluster] = currentCluster + 1;
        }
        currentCluster++;
    }
    nextFreeCluster = currentCluster;
    currentCluster--;
    fat0[currentCluster] = FAT32_EOC_MARK;
    fat1[currentCluster] = FAT32_EOC_MARK;

    return ret;
}

BYTE* Fat::getPointerToCluster(DWORD cluster)
{
    if (cluster < 2)
        return nullptr;
    return dataStart + (cluster - 2) * sectorsPerCluster * SECTOR_SIZE;
}

std::pair<DWORD, DWORD> Fat::writeFile(std::string const& sourcePath)
{
    std::ifstream in(sourcePath, std::ios::in | std::ios::ate | std::ios::binary);

    QWORD qFileSize = in.tellg();
    in.seekg(0);
    if (qFileSize >= UINT32_MAX)
        return std::make_pair(UINT32_MAX, 0);

    DWORD fileSize = static_cast<DWORD>(qFileSize);
    DWORD bytesToWrite = fileSize;
    DWORD clusterCount = fileSize / clusterSize + (fileSize % clusterSize ? 1 : 0); 

    DWORD firstCluster = allocateClusters(UINT32_MAX, clusterCount);  
    if (firstCluster == UINT32_MAX)
        return std::make_pair(UINT32_MAX, 0);

    DWORD currentCluster = firstCluster;    
    BYTE *ptr;
    while (bytesToWrite >= clusterSize)
    {
        ptr = getPointerToCluster(currentCluster);
        in.read(reinterpret_cast<char*>(ptr), clusterSize);
        bytesToWrite -= clusterSize;
        currentCluster = fat0[currentCluster];
    }

    assert(fat0[currentCluster] == FAT32_EOC_MARK);
    ptr = getPointerToCluster(currentCluster);
    in.read(reinterpret_cast<char*>(ptr), bytesToWrite);

    in.close();
    return std::make_pair(firstCluster, fileSize);
}

bool Fat::writeDirectoryEntries(FatRawDirectory &directory, std::unique_ptr<BYTE[]> &entryBuffer, DWORD entryBufferSize)
{
    DWORD newDirectoryEntryCount = entryBufferSize / sizeof(DIR_ENTRY);
    DWORD newDirEntriesIndx = 0;
    DIR_ENTRY *newDirEntries = reinterpret_cast<DIR_ENTRY*>(entryBuffer.get());
    DIR_ENTRY *dirEntries = reinterpret_cast<DIR_ENTRY*>(getPointerToCluster(directory.cluster));
    // Fill current cluster
    for (; directory.entryIndex < maxDirEntries && newDirEntriesIndx < newDirectoryEntryCount; directory.entryIndex++, newDirEntriesIndx++)
        std::memcpy(&(dirEntries[directory.entryIndex]), &(newDirEntries[newDirEntriesIndx]), sizeof(DIR_ENTRY)); 

    // Allocate clusters until we are done
    while (newDirEntriesIndx < newDirectoryEntryCount)
    {
        DWORD newCluster = allocateClusters(directory.cluster, 1);
        if (newCluster == UINT32_MAX)
            return false;
        directory.cluster = newCluster;
        directory.entryIndex = 0;
        dirEntries = reinterpret_cast<DIR_ENTRY*>(getPointerToCluster(newCluster));
        for (; directory.entryIndex < maxDirEntries && newDirEntriesIndx < newDirectoryEntryCount; directory.entryIndex++, newDirEntriesIndx++)
            std::memcpy(&(dirEntries[directory.entryIndex]), &(newDirEntries[newDirEntriesIndx]), sizeof(DIR_ENTRY));
    } 

    return true;
}

bool Fat::createRawFile(FatRawDirectory &directory, std::string const& filename, std::string const& sourcePath)
{
    auto loadedFile = writeFile(sourcePath); 
    if (loadedFile.first == UINT32_MAX)
         return false;

    DWORD entryBufferSize;
    auto entryBuffer = getDirectoryEntry(filename, false, loadedFile.first, loadedFile.second, {}, entryBufferSize);
    if (!entryBuffer.get())
        return false;
   
    return writeDirectoryEntries(directory, entryBuffer, entryBufferSize);
}

std::optional<Fat::FatRawDirectory> Fat::createRawDirectory(FatRawDirectory &parent, std::string const& directoryName)
{
    // Prepare . and .. for the new directory
    DWORD newCluster = allocateClusters(UINT32_MAX, 1);  
    if (newCluster == UINT32_MAX)
        return {};
    
    auto dateTime = getCurrentDateAndTime();
    DIR_ENTRY *dirEntries = reinterpret_cast<DIR_ENTRY*>(getPointerToCluster(newCluster));    


    // Create .
    std::memcpy(dirEntries[0].DIR_Name, ".          ", 11);
    dirEntries[0].DIR_Attr = ATTR_DIRECTORY;
    dirEntries[0].DIR_NTRes = 0;
    dirEntries[0].DIR_CrtTimeTenth = 0;
    dirEntries[0].DIR_CrtTime = dateTime.second;
    dirEntries[0].DIR_CrtDate = dateTime.first;
    dirEntries[0].DIR_LstAccDate = dateTime.first;
    dirEntries[0].DIR_FstClusHI = newCluster >> 16;
    dirEntries[0].DIR_WrtTime = dateTime.second;
    dirEntries[0].DIR_WrtDate = dateTime.first;
    dirEntries[0].DIR_FstClusLO = newCluster & UINT16_MAX;
    dirEntries[0].DIR_FileSize = 0;

    // Create ..
    std::memcpy(dirEntries[1].DIR_Name, "..         ", 11);
    dirEntries[1].DIR_Attr = ATTR_DIRECTORY;
    dirEntries[1].DIR_NTRes = 0;
    dirEntries[1].DIR_CrtTimeTenth = 0;
    dirEntries[1].DIR_CrtTime = dateTime.second;
    dirEntries[1].DIR_CrtDate = dateTime.first;
    dirEntries[1].DIR_LstAccDate = dateTime.first;
    dirEntries[1].DIR_FstClusHI = parent.self ? parent.self->DIR_FstClusHI : 0;
    dirEntries[1].DIR_WrtTime = dateTime.second;
    dirEntries[1].DIR_WrtDate = dateTime.first;
    dirEntries[1].DIR_FstClusLO = parent.self ? parent.self->DIR_FstClusLO : 0;
    dirEntries[1].DIR_FileSize = 0;

    // Create directory entry in the parent
    DWORD entryBufferSize;
    auto entryBuffer = getDirectoryEntry(directoryName, true, newCluster, 0, dateTime, entryBufferSize);
    if (!entryBuffer.get())
        return {};
    
    if (!writeDirectoryEntries(parent, entryBuffer, entryBufferSize))
        return {};
    
    // Create the raw object representing the new directory
    FatRawDirectory ret;
    ret.self = &((reinterpret_cast<DIR_ENTRY*>(getPointerToCluster(parent.cluster)))[parent.entryIndex - 1]);
    ret.cluster = newCluster;
    ret.entryIndex = 2;

    return ret;
}

Fat::FatDirectory* Fat::findDirectory(std::string const& path)
{
    if (path.empty())
        return &rootDirectory;

    std::stringstream ss(path);
    std::string dir;
    FatDirectory *cwd = &rootDirectory;

    while(!ss.eof())
    {
        std::getline(ss, dir, '/');
        auto it = cwd->children.find(dir);
        if (it == cwd->children.end())
            return nullptr;
        cwd = &(it->second);
    }

    return cwd;
}

bool Fat::createDirectory(std::string const& path)
{
    size_t lastSlash = path.find_last_of('/');
    std::string parent;
    if (lastSlash == 0)
        parent = "";
    else
        parent = path.substr(1, lastSlash - 1);
    std::string name = path.substr(lastSlash + 1, std::string::npos);

    FatDirectory *pDir = findDirectory(parent);
    if (!pDir)
        return false;

    std::optional<FatRawDirectory> rawDirOpt = createRawDirectory(pDir->rawDirectory, name);
    if (!rawDirOpt.has_value())
        return false;

    pDir->children[name].rawDirectory = rawDirOpt.value();

    return true;
}

bool Fat::createFile(std::string const& destinationPath, std::string const& sourcePath)
{
    size_t lastSlash = destinationPath.find_last_of('/');
    std::string parent;
    if (lastSlash == 0)
        parent = "";
    else
        parent = destinationPath.substr(1, lastSlash - 1);
    std::string name = destinationPath.substr(lastSlash + 1, std::string::npos);

    FatDirectory *pDir = findDirectory(parent);
    if (!pDir)
        return false;

    return createRawFile(pDir->rawDirectory, name, sourcePath);
}
