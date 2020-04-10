#include <stdio.h>
#include <string.h>
#include "Helpers.h"

int min(int a, int b)
{
	return a < b ? a : b;
}

void get_dir_name(char *full_path, char *dir_path, char *file_name)
{
	strcpy(dir_path, full_path);
	for (int i = strlen(full_path) - 1; i >= 0; i--)
	{
		if (full_path[i] == '/')
		{
			dir_path[i] = 0;
			strcpy(file_name, full_path + i);
		}
	}
}

void invert(bitmap_t *bitmap, int index)
{
	int integer_index = index / (sizeof(int) * 8);
	bitmap->integer[integer_index] ^= (1 << (index % (sizeof(int) * 8)));
}

void invert_bitmap(int sector_index, int index)
{
	char char_array[SECTOR_SIZE];
	Disk_Read(sector_index, char_array);
	bitmap_t *bitmap = (bitmap_t *)(char_array);

	invert(bitmap, index);
	Disk_Write(sector_index, char_array);
	Disk_Read(sector_index, char_array);
	bitmap = (bitmap_t *)(char_array);
	return;
}

char is_set(bitmap_t *bitmap, int index)
{
	int curr_index = 0;
	while (index >= (curr_index + 1) * sizeof(int) * 8)
	{
		curr_index++;
	}
	return (bitmap->integer[curr_index] & (1 << (index % (sizeof(int) * 8)))) != 0;
}

int index_of_block(int block_no, inode_t inode)
{
	for (int i = 0; i < MAX_FILE_SIZE; i++)
	{
		if (inode.blocks[i] == block_no)
		{
			return i;
		}
	}
	return MAX_FILE_SIZE;
}

int first_free_file()
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (file_table_element[i] == NULL)
		{
			return i;
		}
	}
	return MAX_OPEN_FILES;
}

int get_inode_block(int index)
{
	int inodes_per_sector = SECTOR_SIZE / sizeof(inode_t);
	return index / inodes_per_sector;
}

inode_t get_inode(int index)
{
	int curr_index = get_inode_block(index);
	int inodes_per_sector = SECTOR_SIZE / sizeof(inode_t);
	char char_array[SECTOR_SIZE];
	Disk_Read(3 + curr_index, char_array);
	return *(inode_t *)(char_array + ((index) % inodes_per_sector) * sizeof(inode_t));
}

int find_inode(char *dir_name, char inode_index)
{
	inode_t inode = get_inode(inode_index);

	if (inode.is_file != 0)
	{
		return -1;
	}

	for (int i = 0; i < MAX_FILE_SIZE && inode.blocks[i] != 0; i++)
	{
		char char_array[SECTOR_SIZE];
		Disk_Read(inode.blocks[i], char_array);
		directory_block_t *directory_block = (directory_block_t *)char_array;

		for (int j = 0; j < SECTOR_SIZE / sizeof(struct files) - 2; j++)
		{
			if (directory_block->file[j].file_name[0] == 0)
			{
				return -1;
			}
			if (strcmp(dir_name, (directory_block->file[j]).file_name) == 0)
			{
				return directory_block->file[j].inode;
			}
		}
	}
	return -1;
}

int empty_block_index(inode_t inode)
{
	for (int i = 0; i < MAX_FILE_SIZE; i++)
	{
		if (inode.blocks[i] == 0)
			return i;
	}
	return MAX_FILE_SIZE;
}

int get_inode_from_path(char *file)
{
	char dir_name[16];
	int curr_index = 1;		  //to avoid '/' in the beginning
	int curr_inode_index = 0; //root inode
	while (curr_inode_index != -1)
	{
		int j, i;
		for (i = curr_index, j = 0; i < strlen(file) && file[i] != '/'; i++, j++)
		{
			dir_name[j] = file[curr_index];
		}
		dir_name[j] = 0;

		curr_inode_index = find_inode(dir_name, curr_inode_index);
		if (i == strlen(file))
		{
			return curr_inode_index;
		}
	}
	return -1;
}

int get_smallest_in_bitmap(int sector_index)
{
	char char_array[SECTOR_SIZE];
	Disk_Read(sector_index, char_array);
	bitmap_t *bitmap = (bitmap_t *)char_array;
	int max_index = (sector_index == INODE_BITMAP ? MAX_INODES : NUM_SECTORS);

	for (int i = 0; i < max_index; i++)
	{
		if (!is_set(bitmap, i))
		{
			return i;
		}
	}
	return max_index;
}

void extract_names(char *full_path, char *dir_path, char *file_path)
{
	int index = strlen(full_path) - 1;
	while (full_path[index] != '/')
	{
		index--;
	}
	strcpy(file_path, full_path + index + 1);
	full_path[index] = 0;
	strcpy(dir_path, full_path);
}

void insert_into_directory_block(int block_no, int file_no, int inode_index, char *file)
{
	directory_block_t *db;
	char char_array[SECTOR_SIZE];
	Disk_Read(block_no, char_array);
	db = (directory_block_t *)char_array;
	if (file_no == 0)
	{
		memset(db, sizeof(db), 0);
	}

	db->file[file_no].inode = inode_index;
	strcpy(db->file[file_no].file_name, file);
	memcpy(char_array, &db, sizeof(directory_block_t));
	Disk_Write(block_no, char_array);
}

void insert_file_in_directory(int dir_inode_index, char *file, int file_inode_index)
{
	int curr_index = -1, block_no, file_no = -1;

	inode_t dir_inode = get_inode(dir_inode_index);
	while (dir_inode.blocks[curr_index + 1] != 0)
	{
		curr_index++;
	}
	if (curr_index != -1)
	{
		block_no = dir_inode.blocks[curr_index];
		char char_array[SECTOR_SIZE];
		Disk_Read(curr_index, char_array);
		directory_block_t *db = (directory_block_t *)(char_array);

		int i;
		for (i = 0; i < SECTOR_SIZE / sizeof(struct files) - 2; i++)
		{
			if (db->file[i].inode == 0)
			{
				file_no = i;
				break;
			}
		}
	}

	if (file_no == -1)
	{
		block_no = get_smallest_in_bitmap(DATA_BITMAP);
		invert_bitmap(DATA_BITMAP, block_no);
		dir_inode.blocks[curr_index + 1] = block_no;
		file_no = 0;
	}

	insert_into_directory_block(block_no, file_no, file_inode_index, file);
}

void save_inode(inode_t in, int inode_index)
{
	char block_no = get_inode_block(inode_index);
	char char_array[SECTOR_SIZE];
	Disk_Read(block_no, char_array);

	int inodes_per_sector = SECTOR_SIZE / sizeof(inode_t);
	inode_t *temp = (inode_t *)(char_array + ((inode_index) % inodes_per_sector) * sizeof(inode_t));
	temp->size = in.size;
	temp->is_file = in.is_file;
	for (int i = 0; i < MAX_FILE_SIZE; i++)
	{
		temp->blocks[i] = in.blocks[i];
	}
	Disk_Write(block_no, char_array);
}

void insert_file_in_inode(char *file, int inode_index)
{
	invert_bitmap(INODE_BITMAP, inode_index);
	inode_t in = {0};
	in.is_file = 1;
	save_inode(in, inode_index);
}

int per_file_index(int inode)
{
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (per_file_table[i] != NULL)
		{
			if (per_file_table[i]->inode == inode)
			{
				return i;
			}
		}
	}
	return -1;
}

void update_per_file_table(int inode, int change)
{
	int index = per_file_index(inode);
	if (index != -1)
	{
		per_file_table[index]->open_files += change;
		if (per_file_table[index]->open_files == 0)
		{
			free(per_file_table[index]);
			per_file_table[index] = NULL;
		}
		return;
	}

	//if the file is unused till now
	for (int i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (per_file_table[i] == NULL)
		{
			per_file_table[i] = (per_file_table_t *)malloc(sizeof(per_file_table_t));
			per_file_table[i]->inode = inode, per_file_table[i]->open_files = 1;
			return;
		}
	}
}

void delete_inode(int inode_index)
{
	inode_t inode = get_inode(inode_index);
	char char_array[SECTOR_SIZE];
	Disk_Read(DATA_BITMAP, char_array);
	bitmap_t *bitmap = (bitmap_t *)char_array;

	for (int i = 0; i < MAX_FILE_SIZE || inode.blocks[i] == 0; i++)
	{
		invert(bitmap, inode.blocks[i]);
	}
	Disk_Write(DATA_BITMAP, char_array);

	Disk_Read(INODE_BITMAP, char_array);
	bitmap_t *bitmap = (bitmap_t *)char_array;
	invert(bitmap, inode_index);
	Disk_Write(INODE_BITMAP, char_array);
}

void delete_file_from_directory(int dir_inode_index, int inode)
{
	inode_t dir_inode = get_inode(dir_inode_index);

	int curr_index = 0;
	while (dir_inode.blocks[curr_index] != 0)
	{
		char char_array[SECTOR_SIZE];
		Disk_Read(dir_inode.blocks[curr_index], char_array);
		directory_block_t *db = (directory_block_t *)char_array;
		int flag = 0;

		for (int i = 0; i < SECTOR_SIZE / sizeof(struct files) - 2; i++)
		{
			if (db->file[i].inode == inode)
			{
				db->file[i].inode = 0;
				flag = 1;
				break;
			}
		}

		if (flag)
		{
			Disk_Write(dir_inode.blocks[curr_index], char_array);
			break;
		}
	}
}
