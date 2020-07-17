
#ifndef __HELPERS__
#define __HELPERS__

#include "LibDisk.h"

#define DISK_IMAGE_NAME_SIZE 30
#define MAGIC_NUMBER 2
#define MAX_FILE_SIZE 30 //blocks
#define MAX_INODES 100
#define MAX_FILE_NAME 16
#define MAX_PATH_LENGTH 256
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
	int curr_block_no; //block no, not the index of the block
	int pos;		   //position from the beginning of the file,not the position in the block
};

struct per_file_table
{
	int inode;
	int open_files;
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
typedef struct per_file_table per_file_table_t;

file_table_element_t *file_table_element[MAX_OPEN_FILES];
per_file_table_t *per_file_table[MAX_OPEN_FILES];

int min(int a, int b);
int max(int a, int b);
void get_dir_name(char *full_path, char *dir_path, char *file_name);
void invert(bitmap_t *bitmap, int index);
void invert_bitmap(int sector_index, int index);
char is_set(bitmap_t *bitmap, int index);
int index_of_block(int block_no, inode_t inode);
int first_free_file();
int get_inode_block(int index);
inode_t get_inode(int index);
int find_inode(char *dir_name, char inode_index);
int empty_block_index(inode_t inode);
int get_inode_from_path(char *file);
int get_smallest_in_bitmap(int sector_index);
void extract_names(char *full_path, char *dir_path, char *file_path);
void insert_into_directory_block(int block_no, int file_no, int inode_index, char *file);
void save_inode(inode_t in, int inode_index);
void insert_into_directory(int dir_inode_index, char *file, int file_inode_index);
void new_inode(int inode_index, int is_file);
int per_file_index(int inode);
void update_per_file_table(int inode, int change);
void delete_inode(int inode_index);
void delete_file_from_directory(int dir_inode_index, int inode);
void to_char(int num, char *ch_arr);
int files_in_inode(inode_t inode);

#endif
