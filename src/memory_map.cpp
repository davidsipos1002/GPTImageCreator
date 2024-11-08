#include <memory_map.hpp>

#include <cal_types.h>

#ifdef _WIN32

#include <Windows.h>
typedef struct
{
    HANDLE fileHandle;
    HANDLE fileMapping;
    void *address;
} MemoryMapping;

void* openMemoryMappedFile(MemoryMappedFile *file, const char *path)
{
    MemoryMapping **mapping = (MemoryMapping **) file;
    *mapping = NULL;

    HANDLE fileHandle = CreateFileA(path, 
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (fileHandle == INVALID_HANDLE_VALUE)
        return NULL;

    HANDLE fileMapping = CreateFileMappingA(fileHandle,
        NULL,
        PAGE_READWRITE,
        0,
        0,
        path
    );

    if (fileMapping == INVALID_HANDLE_VALUE)
    {
        CloseHandle(fileHandle);
        return NULL;
    }

    void *address = MapViewOfFile(fileMapping,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0,
        0,
        0);

    if (!address)
    {
        CloseHandle(fileMapping);
        CloseHandle(fileHandle);
        return NULL;
    }

    MemoryMapping *ret = (MemoryMapping *) malloc(sizeof(MemoryMapping));
    ret->fileHandle = fileHandle;
    ret->fileMapping = fileMapping;
    ret->address = address;
    *mapping = ret;

    return address;
}

void closeMemoryMappedFile(MemoryMappedFile *file)
{
    MemoryMapping **mapping = (MemoryMapping **) file;
    MemoryMapping *map = *mapping;
    UnmapViewOfFile(map->address);
    CloseHandle(map->fileMapping);
    CloseHandle(map->fileHandle);
    free(map);
    *mapping = NULL;
}

#else

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct
{
    int fd;
    uint64_t size;
    void *address;
} MemoryMapping;

void *openMemoryMappedFile(MemoryMappedFile *file, const char *path)
{
    MemoryMapping **mapping = (MemoryMapping **) file;
    *mapping = NULL;

    int fd = open(path, O_RDWR);
    if (fd == -1)
        return NULL;

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        close(fd);
        return NULL;
    }

    void *address = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address == MAP_FAILED)
    {
        close(fd);
        return NULL;
    }

    MemoryMapping *ret = (MemoryMapping *) malloc(sizeof(MemoryMapping));
    ret->fd = fd;
    ret->size = sb.st_size;
    ret->address = address;
    *mapping = ret;
    
    return address;
}

void closeMemoryMappedFile(MemoryMappedFile *file)
{
    MemoryMapping **mapping = (MemoryMapping **) file;
    MemoryMapping *map = *mapping;

    munmap(map->address, map->size);
    close(map->fd);
    free(map);

    *mapping = NULL;
}

#endif
