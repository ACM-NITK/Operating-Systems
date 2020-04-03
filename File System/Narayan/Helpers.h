
#ifndef __HELPERS__
#define __HELPERS__

#include "LibDisk.h"

#define DISK_IMAGE_NAME_SIZE 30
#define MAGIC_NUMBER 2
#define MAX_FILE_SIZE 30 //blocks
#define MAX_INODES 100

struct inode
{
	int size;
	char is_file;			   //bool-type, false indicates the inode is of a directory
	int blocks[MAX_FILE_SIZE]; // index of the blocks for the file
};

struct bitmap
{
	int integer[SECTOR_SIZE / sizeof(int) - 2]; // minus 2 for padding with the struct
};

typedef struct inode inode_t;
typedef struct bitmap bitmap_t;

void set(bitmap_t *bitmap, int index);

void unset(bitmap_t *bitmap, int index);

#endif