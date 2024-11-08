#pragma once

typedef void* MemoryMappedFile;

void *openMemoryMappedFile(MemoryMappedFile *file, const char *path);
void closeMemoryMappedFile(MemoryMappedFile *file);
