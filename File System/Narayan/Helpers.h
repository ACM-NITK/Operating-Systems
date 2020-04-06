
#ifndef __HELPERS__
#define __HELPERS__

#include "LibDisk.h"

#define DISK_IMAGE_NAME_SIZE 30
#define MAGIC_NUMBER 2
#define MAX_FILE_SIZE 30 //blocks
#define MAX_INODES 100
#define MAX_FILE_NAME 16
//TODO
#define MAX_OPEN_FILES 100

typedef enum
{
	INODE_BITMAP = 1,
	DATA_BITMAP = 2
} sector_number;

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

struct file_table_element
{
	int inode;
	int curr_block_no;
	int pos;
};

struct directory_block
{
	struct files
	{
		int inode;
		char file_name[16];
	} file[SECTOR_SIZE / sizeof(struct files) - 2]; //minus 2 for padding with the struct
};

typedef struct inode inode_t;
typedef struct bitmap bitmap_t;
typedef struct file_table_element file_table_element_t;
typedef struct directory_block directory_block_t;

file_table_element_t *file_table_element[MAX_OPEN_FILES];

void set(bitmap_t *bitmap, int index);
void unset(bitmap_t *bitmap, int index);
int first_free_file();

#endif
