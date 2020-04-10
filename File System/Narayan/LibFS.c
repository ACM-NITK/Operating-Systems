#include "LibFS.h"
#include "LibDisk.h"
#include "Helpers.c"
#include <string.h>

// global errno value here
int osErrno;
char disk_image_name[DISK_IMAGE_NAME_SIZE];
int BITMAP_SIZE = SECTOR_SIZE / sizeof(int) - 2;

int FS_Boot(char *path)
{
	printf("FS_Boot %s\n", path);

	if (Disk_Init() == -1)
	{
		osErrno = E_GENERAL;
		return -1;
	}

	if (Disk_Load(path) == -1)
	{
		if (diskErrno != E_OPENING_FILE)
		{
			osErrno = E_GENERAL;
			return -1;
		}

		/* if there is was no such file,
		write the magic number in the superblock (block number 0) of the newly initialised file system
		*/
		char super_block[SECTOR_SIZE];
		super_block[0] = MAGIC_NUMBER + '0';
		Disk_Write(0, super_block);

		//initialise MAX_INODES inodes starting from the fourth block
		int curr_inode = 0;
		int curr_block = 3;
		while (curr_inode < MAX_INODES)
		{
			int inodes_per_sector = SECTOR_SIZE / (sizeof(inode_t));
			char curr_sector[SECTOR_SIZE];

			for (int i = 0; i < inodes_per_sector; i++)
			{
				inode_t new_inode = {0};
				memcpy(curr_sector + sizeof(inode_t) * i, &new_inode, sizeof(inode_t));
				curr_inode++;
			}

			Disk_Write(curr_block, curr_sector);
			curr_block++;
		}
		int last_filled_block = curr_block - 1;

		// --------------------------INODE BITMAP----------------------------//

		bitmap_t inode_bitmap = {0};

		//only 1st inode will be occupied for the root directory
		invert(&inode_bitmap, 0);

		//write the struct into a character array
		char char_array[SECTOR_SIZE];
		memcpy(char_array, &inode_bitmap, sizeof(bitmap_t));

		Disk_Write(INODE_BITMAP, char_array);

		// ------------------------DATABLOCK BITMAP---------------------------//

		bitmap_t datablock_bitmap = {0};

		//set the used datablocks in the bitmap
		for (int i = 0; i <= last_filled_block; i++)
		{
			invert(&datablock_bitmap, i);
		}

		//write the struct into a character array
		memcpy(char_array, &datablock_bitmap, sizeof(bitmap_t));

		Disk_Write(DATA_BITMAP, char_array);
	}
	else
	{
		//check for the value in the supernode
		char super_block[SECTOR_SIZE];
		Disk_Read(0, super_block);

		if (super_block[0] != MAGIC_NUMBER + '0')
		{
			osErrno = E_GENERAL;
			return -1;
		}
	}
	strcpy(disk_image_name, path);
	Disk_Save(disk_image_name);
	return 0;
}

int FS_Sync()
{
	printf("FS_Sync\n");
	if (Disk_Save(disk_image_name) == -1)
	{
		osErrno = E_GENERAL;
		return -1;
	}
	return 0;
}

int File_Create(char *full_path)
{
	printf("FS_Create\n");

	char dir_path[MAX_PATH_LENGTH], file_name[MAX_FILE_NAME];
	extract_names(full_path, dir_path, file_name);
	int dir_inode_index = get_inode_from_path(dir_path);

	if (dir_inode_index == -1)
	{
		osErrno = E_CREATE;
		return -1;
	}
	int file_inode_index = get_smallest_in_bitmap(INODE_BITMAP);
	if (file_inode_index == MAX_INODES)
	{
		osErrno = E_CREATE;
		return -1;
	}

	insert_file_in_directory(dir_inode_index, file_name, file_inode_index);
	insert_file_in_inode(file_name, file_inode_index);
	return 0;
}

int File_Open(char *file)
{
	printf("FS_Open\n");
	int fd = first_free_file();
	int inode = get_inode_from_path(file);
	file_table_element[fd] = (file_table_element_t *)malloc(sizeof(file_table_element_t));
	file_table_element[fd]->inode = inode;
	file_table_element[fd]->curr_block_no = get_inode(inode).blocks[0];
	// when curr_block_no is 0, it is not allocated, hence needs to be allocated when writing
	file_table_element[fd]->pos = 0;

	update_per_file_table(inode, 1);
	return 0;
}

int File_Read(int fd, void *buffer, int size)
{
	printf("FS_Read\n");
	if (file_table_element[fd] == NULL)
	{
		osErrno = E_BAD_FD;
		return -1;
	}
	inode_t inode = get_inode(file_table_element[fd]->inode);
	char char_array[SECTOR_SIZE + 1];
	int chars_read = 0;

	while (file_table_element[fd]->pos < inode.size && chars_read < size)
	{
		Disk_Read(file_table_element[fd]->curr_block_no, char_array);

		int chars_left = SECTOR_SIZE - (file_table_element[fd]->pos % SECTOR_SIZE);
		if (chars_read + chars_left < size && (file_table_element[fd]->pos + chars_left < inode.size))
		{
			strcpy(buffer + chars_read, char_array + (SECTOR_SIZE - chars_left));
			file_table_element[fd]->pos += chars_left;
			file_table_element[fd]->curr_block_no = inode.blocks[index_of_block(file_table_element[fd]->curr_block_no, inode) + 1];
			chars_read += chars_left;
		}
		else if (chars_read + chars_left < size)
		{
			strcpy(buffer + chars_read, char_array + (SECTOR_SIZE - chars_left));
			file_table_element[fd]->pos = inode.size;
			chars_read += inode.size - (file_table_element[fd]->pos + chars_left);
		}
		else
		{
			int chars_to_be_read = min(inode.size - file_table_element[fd]->pos, size - chars_read);
			char_array[SECTOR_SIZE - chars_left + chars_to_be_read] = 0; //null termination of a string
			strcpy(buffer + chars_read, char_array);
			file_table_element[fd]->pos = inode.size;
			chars_read = size;
		}
	}
	return chars_read;
}

int File_Write(int fd, void *buffer, int size)
{
	printf("FS_Write\n");
	if (file_table_element[fd] == NULL)
	{
		osErrno = E_BAD_FD;
		return -1;
	}
	inode_t inode = get_inode(file_table_element[fd]->inode);
	char char_array[SECTOR_SIZE];
	int chars_written = 0;

	while (file_table_element[fd]->pos < inode.size && chars_written < size)
	{
		if (file_table_element[fd]->curr_block_no == 0)
		{
			int index = get_smallest_in_bitmap(DATA_BITMAP);
			if (index == NUM_SECTORS)
			{
				osErrno = E_NO_SPACE;
				return -1;
			}
			if (empty_block_index(inode) == MAX_FILE_SIZE)
			{
				osErrno = E_FILE_TOO_BIG;
			}
			file_table_element[fd]->curr_block_no = index;
			inode.blocks[empty_block_index(inode)] = index;
			save_inode(inode, file_table_element[fd]->inode);
			invert_bitmap(DATA_BITMAP, file_table_element[fd]->curr_block_no);
		}

		Disk_Read(file_table_element[fd]->curr_block_no, char_array);

		int chars_left = SECTOR_SIZE - (file_table_element[fd]->pos % SECTOR_SIZE);
		if (chars_written + chars_left < size && (file_table_element[fd]->pos + chars_left < inode.size))
		{
			strcpy(char_array + (SECTOR_SIZE - chars_left), buffer + chars_written);
			Disk_Write(file_table_element[fd]->curr_block_no, char_array);

			file_table_element[fd]->pos += chars_left;
			file_table_element[fd]->curr_block_no = inode.blocks[index_of_block(file_table_element[fd]->curr_block_no, inode) + 1];
			chars_written += chars_left;
		}
		else if (chars_written + chars_left < size)
		{
			strcpy(char_array + (SECTOR_SIZE - chars_left), buffer + chars_written);
			Disk_Write(file_table_element[fd]->curr_block_no, char_array);

			inode.size += (file_table_element[fd]->pos + chars_left - inode.size);
			save_inode(inode, file_table_element[fd]->inode);

			file_table_element[fd]->pos = inode.size;
			file_table_element[fd]->curr_block_no = 0;
			chars_written += inode.size - (file_table_element[fd]->pos + chars_left);
		}
		else
		{
			strcpy(char_array + (SECTOR_SIZE - chars_left), buffer + chars_written);
			Disk_Write(file_table_element[fd]->curr_block_no, char_array);

			file_table_element[fd]->pos += size - chars_written;
			chars_written = size;

			inode.size = file_table_element[fd]->pos;
			save_inode(inode, file_table_element[fd]->inode);
		}
	}
	return size;
}

int File_Seek(int fd, int offset)
{
	printf("FS_Seek\n");

	if (file_table_element[fd] == NULL)
	{
		osErrno = E_BAD_FD;
		return -1;
	}
	inode_t inode = get_inode(file_table_element[fd]->inode);
	if (offset < 0 || offset >= inode.size)
	{
		osErrno = E_SEEK_OUT_OF_BOUNDS;
		return -1;
	}

	file_table_element[fd]->curr_block_no = inode.blocks[offset / SECTOR_SIZE];
	file_table_element[fd]->pos = offset;

	return 0;
}

int File_Close(int fd)
{
	printf("FS_Close\n");

	if (file_table_element[fd] == NULL)
	{
		osErrno = E_BAD_FD;
		return -1;
	}

	update_per_file_table(file_table_element[fd]->inode, -1);

	free(file_table_element[fd]);
	file_table_element[fd] = NULL;

	return 0;
}

int File_Unlink(char *full_path)
{
	printf("FS_Unlink\n");

	int inode = get_inode_from_path(full_path);

	if (inode == -1)
	{
		osErrno = E_NO_SUCH_FILE;
		return -1;
	}

	if (per_file_index(inode) != -1)
	{
		osErrno = E_FILE_IN_USE;
		return -1;
	}

	char dir_path[MAX_PATH_LENGTH], file_name[MAX_FILE_NAME];
	extract_names(full_path, dir_path, file_name);
	int dir_inode_index = get_inode_from_path(dir_path);

	delete_inode(inode);
	delete_file_from_directory(dir_inode_index, inode);

	return 0;
}

// directory ops
int Dir_Create(char *path)
{
	printf("Dir_Create %s\n", path);
	return 0;
}

int Dir_Size(char *path)
{
	printf("Dir_Size\n");
	return 0;
}

int Dir_Read(char *path, void *buffer, int size)
{
	printf("Dir_Read\n");
	return 0;
}

int Dir_Unlink(char *path)
{
	printf("Dir_Unlink\n");
	return 0;
}

int main()
{
	char path[15] = "hi.txt";
	printf("FSBdeddeOot %d\n", FS_Boot(path));
	printf("bitmap%d\n", get_smallest_in_bitmap(INODE_BITMAP));
	File_Create("/hel.txt");

	int fd = File_Open("/hel.txt");
	char char_array[100];
	File_Read(fd, char_array, 2);
	printf("%s", char_array);

	char char_array2[100] = "wefwew";
	File_Write(fd, char_array2, 5);

	File_Read(fd, char_array, 2);
	printf("%s", char_array);

	printf("bitmap%d\n", get_smallest_in_bitmap(INODE_BITMAP));

	return 0;
}