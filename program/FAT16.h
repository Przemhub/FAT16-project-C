#ifndef FAT16_H_INCLUDED
#define FAT16_H_INCLUDED

#include <stdio.h>

#pragma pack(push, 1)

typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short sector_size;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char number_of_fats;
    unsigned short root_dir_entries;
    unsigned short total_sectors_short; 
    unsigned char media_descriptor;
    unsigned short fat_size_sectors;
    unsigned short sectors_per_track;
    unsigned short number_of_heads;
    unsigned long hidden_sectors;
    unsigned long total_sectors_long;

    unsigned char drive_number;
    unsigned char current_head;
    unsigned char boot_signature;
    unsigned long volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed)) Fat16BootSector;

typedef struct {
    unsigned char filename[8];
    unsigned char ext[3];
    unsigned char attributes;
    unsigned char reserved[10];
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned short starting_cluster;
    unsigned long file_size;
} __attribute((packed)) Fat16Entry;

#pragma pack(pop)

typedef struct {
    Fat16BootSector bs;
    FILE *file;
    Fat16Entry currentDirEntry;
    unsigned long fatStart;
    unsigned long rootStart;
    unsigned long dataStart;
} fatFile;

typedef enum {
    ENTRY_FREE,
    ENTRY_DIR,
    ENTRY_FILE,
} entryType;

int openFat16File(fatFile *fat, char *filename);
void closeFat16File(fatFile *fat);
void print_file_info(Fat16Entry *entry);
Fat16Entry *findEntry(fatFile *fat, char *entryName, entryType type);
int changeDir(fatFile *fat, char *dirname);
void gotoRootDir(fatFile *fat);
int readFile(fatFile *fat, char *filename, char **content);
int saveNewFile(fatFile *fat, char *filename, char *content);
unsigned short allocateNewCluster(fatFile *fat, unsigned short lastCluster);


#endif // FAT16_H_INCLUDED
