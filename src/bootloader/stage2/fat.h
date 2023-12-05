//
// Created by caleb on 10/31/2023.
//
/*
#pragma once

#include "stdint.h"
#include "disk.h"

#pragma pack(push, 1)

typedef struct {
    uint8_t BootJumpInstructions[3];
    uint8_t OemIdentifier[8];
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fatCount;
    uint16_t dirEntryCount;
    uint16_t totalSectors;
    uint8_t mediaDescriptorType;
    uint16_t sectorsPerFat;
    uint16_t sectorsPerTrack;
    uint16_t heads;
    uint16_t hiddenSectors;
    uint32_t largeSectorCount;

    // extended boot record
    uint8_t driveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId; // serial number, value doesn't matter
    uint8_t VolumeLabel[11]; // 11 bytes, padded w/ spaces
    uint8_t SystemId[8];
} FAT_DirectoryEntry;

#pragma pack(pop)
uint16_t bytesPerSector;

typedef struct
{
    int Handle;
    bool isDirectory;
    uint32_t Position;
    uint32_t Size;
} FAT_File;

bool FAT_Initialize(DISK* disk);
FAT_File FAT_Open(DISK* disk, const char* path);
uint32_t FAT_Read(DISK* disk, FAT_File far* file, uint32_t byteCount, void* dataOut);
bool FAT_ReadEntry(DISK* disk, FAT_File far* file, FAT_DirectoryEntry* dirEntry);
void FAT_Close(FAT_File far* file);
/**/
