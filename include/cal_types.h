#pragma once

#include <cstdint>

// Now define those ugly Windows which I don't like at all
// Definition taken from MS Documentation

typedef uint16_t CHAR16, *PCHAR16;
typedef uint64_t QWORD, *PQWORD;

#ifndef _WIN32

typedef int32_t BOOL, *PBOOL;
typedef uint8_t BOOLEAN, *PBOOLEAN;
typedef uint8_t BYTE, *PBYTE, *LPBYTE;
typedef char CHAR8, *PCHAR8;
typedef double DOUBLE, *PDOUBLE;
typedef uint32_t DWORD, *PDWORD;
typedef uint32_t DWORD32, *PDWORD32;
typedef uint64_t DWORD64, *PDWORD64;
typedef float FLOAT, *PFLOAT;
typedef int32_t INT, *PINT;
typedef int8_t INT8, *PINT8; 
typedef int16_t INT16, *PINT16;
typedef int32_t INT32, *PINT32;
typedef int64_t INT64, *PINT64;
typedef int64_t INTN, *PINTN;
typedef int32_t LONG, *PLONG;
typedef int64_t LONGLONG, *PLONGLONG;
typedef int32_t LONG32, *PLONG32;
typedef int64_t LONG64, *PLONG64;
typedef int16_t SHORT, *PSHORT;
typedef uint64_t SIZE_T, *PSIZE_T;
typedef uint8_t *STRING;
typedef uint8_t UCHAR, *PUCHAR;
typedef uint8_t UINT8, *PUINT8;
typedef uint16_t UINT16, *PUINT16;
typedef uint32_t UINT32, *PUINT32;
typedef uint64_t UINT64, *PUINT64;
typedef uint64_t UINTN, *PUINTN;
typedef uint32_t ULONG, *PULONG;
typedef uint32_t ULONG32, *PULONG32;
typedef uint64_t ULONG64, *PULONG64;
typedef uint64_t ULONGLONG, *PULONGLONG;
typedef uint16_t USHORT, *PUSHORT;
typedef void VOID, *PVOID;
typedef uint16_t WCHAR, *PWCHAR;
typedef uint16_t WORD, *PWORD;

#else

#include <minwindef.h>

#endif