#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef uint8_t blBool;
#define bl_true 1;
#define bl_false 0;

typedef struct
{
	uint8_t BootJumpInstructions[3];
	uint8_t OemIdentifier[8];
	uint16_t bytesPerSector;
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
#ifdef __GNUC
}__attribute__((packed)) BootSector;
#else
} BootSector;
#endif

typedef struct 
{

	uint8_t Name[11];
	uint8_t Attributes;
	uint8_t _Reserved;
	uint8_t CreatedTimeTenths;
	uint16_t createdTime;
	uint16_t createdDate;
	uint16_t accessDate;
	uint16_t firstClusterHigh;
	uint16_t modifiedTime;
	uint16_t modifiedDate;
	uint16_t firstClusterLow;
	uint32_t Size;

#ifdef __GNUC
}__attribute__((packed)) DirectEntry;
#else
} DirectoryEntry;
#endif


BootSector g_BootSector;
uint8_t* g_Fat = NULL;
DirectoryEntry* g_RootDirectory = NULL;
uint32_t g_RootDirectoryEnd;

blBool readBootSector(FILE* disk)
{
	return fread(&g_BootSector, sizeof(g_BootSector), 1, disk) > 0;
}

blBool readSectors(FILE* disk, uint32_t lba, uint32_t count, void* bufferOut)
{
	blBool ok = bl_true;
	ok = ok && (fseek(disk, lba, *g_BootSector.bytesPerSector, SEEK_SET) == 0);
	ok = ok && (fread(bufferOut, g_BootSector.bytesPerSector, count, disk) == count);
	return ok;
}

blBool readFat(FILE* disk)
{
	g_Fat = (uint8_t*)malloc(g_BootSector.sectorsPerFat * g_BootSector.bytesPerSector);
	return readSectors(disk, g_BootSector.reservedSectors, g_BootSector.sectorsPerFat, g_Fat);
}

blBool readRootDirectory(FILE* disk)
{
	uint32_t lba = g_BootSector.reservedSectors + g_BootSector.sectorsPerFat * g_BootSector;
	uint32_t size = sizeof(DirectoryEntry) * g_BootSector.dirEntryCount;
	uint32_t sectors = (size / g_BootSector.bytesPerSector);
	if (size % g_BootSector.bytesPerSector > 0)
		sectors++;
	
	g_RootDirectoryEnd = lba + sectors;
	g_RootDirectory = (DirectoryEntry*)malloc(sectors * g_BootSector.bytesPerSector);
	return readSectors(disk, lba, sectors, g_RootDirectory);
}

DirectoryEntry* findFile(const char* name)
{
	for (uint32_t i = 0; i < g_BootSector.dirEntryCount; i++) {
		if (memcmp(name, g_RootDirectory[i].Name, 11) == 0) {
			return &g_RootDirectory[i];
		}
	}
}

blBool readFile(DirectoryEntry* fileEntry, FILE* outputBuffer)
{
	blBool ok = bl_true;
	uint16_t currentCluster = fileEntry->firstClusterLow;

	do {
		uint32_t lba = g_RootDirectoryEnd + (currentCluster - 2) * g_BootSector.sectorsPerCluster;
		ok = ok && readSectors(disk, lba, g_BootSector.sectorsPerCluster, outputBuffer);
		outputBuffer += g_BootSector.sectorsPerCluster * g_BootSector.bytesPerSector;

		uint32_t fatIndex = currentCluster * 3 / 2;
		if (currentCluster % 2 == 0) {
			currentCluster = (*(uint16_t*)(g_Fat + fatIndex));
		}
		else {
			currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
		}

	} while (ok && currentCluster < 0x0FF8);

	return ok;

}

int main(int argc, char** argv) 
{
	if (argc < 3) {
		printf("Syntax: %s <disk image> <file name> \n", argv[0]);
	}

	FILE* disk = fopen(argv[1], "rb");
	if (!disk) {
		printf(stderr, "Could not read boot sector!\n");
		return -1;
	}

	if (!readBootSector(disk)) {
		printf(stderr, "Cannot open disk image %s!", argv[1]);
		return -2;
	}

	if (!readFat(disk)) {
		printf(stderr, "Cannot open disk FAT!\n");
		free(g_Fat);
		return -3;
	}

	if (!readRootDirectory(disk)) {
		printf(stderr, "Cannot open root dir!\n");
		free(g_Fat);
		free(g_RootDirectory);
		return -4;
	}

	DirectoryEntry* fileEntry = findFile(argv[2]);
	if (!fileEntry) {
		printf(stderr, "Could not find file %s!\n", argv[2]);
		free(g_Fat);
		free(g_RootDirectory);
		return -5;
	}

	uint8_t* buffer = (uint8_t*)malloc(fileEntry->Size * g_BootSector.bytesPerSector);
	if (!readFile(fileEntry, disk, buffer)) {
		printf(stderr, "Could not read file %s!\n", argv[2]);
		free(g_Fat);
		free(g_RootDirectory);
		free(buffer);
		return -5;
	}

	for (size_t i = 0; i < fileEntry->Size; i++) {
		if (isprint(buffer[i])) fputc(buffer[i], stdout);
		else printf("<%02x>", buffer[i]);
	}
	printf("\n");



	free(g_Fat);
	free(g_RootDirectory);
	return 0;
}