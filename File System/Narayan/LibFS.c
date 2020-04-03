#include "LibFS.h"
#include "LibDisk.h"
#include "Helpers.h"
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
				inode_t new_inode;
				new_inode.size = 0;
				memcpy(curr_sector + sizeof(inode_t) * i, &new_inode, sizeof(inode_t));
				curr_inode++;
			}

			Disk_Write(curr_block, curr_sector);
			curr_block++;
		}
		int last_filled_block = curr_block - 1;

		// --------------------------INODE BITMAP----------------------------//

		bitmap_t inode_bitmap;

		//write the struct into a character array
		char char_inode_bitmap[SECTOR_SIZE];
		memcpy(char_inode_bitmap, &inode_bitmap, sizeof(bitmap_t));

		//write into the 2nd block
		Disk_Write(1, char_inode_bitmap);

		// ------------------------DATABLOCK BITMAP---------------------------//

		bitmap_t datablock_bitmap;

		//set the used datablocks in the bitmap
		for (int i = 0; i <= last_filled_block; i++)
		{
			set(&datablock_bitmap, i);
		}

		//write the struct into a character array
		char char_datablock_bitmap[SECTOR_SIZE];
		memcpy(char_datablock_bitmap, &datablock_bitmap, sizeof(bitmap_t));

		//write into the 3nd block
		Disk_Write(2, char_inode_bitmap);
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
		Disk_Read(2, super_block);
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

int File_Create(char *file)
{
	printf("FS_Create\n");

	return 0;
}

int File_Open(char *file)
{
	printf("FS_Open\n");
	return 0;
}

int File_Read(int fd, void *buffer, int size)
{
	printf("FS_Read\n");
	return 0;
}

int File_Write(int fd, void *buffer, int size)
{
	printf("FS_Write\n");
	return 0;
}

int File_Seek(int fd, int offset)
{
	printf("FS_Seek\n");
	return 0;
}

int File_Close(int fd)
{
	printf("FS_Close\n");
	return 0;
}

int File_Unlink(char *file)
{
	printf("FS_Unlink\n");
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
	printf("%d", FS_Boot(path));
	return 0;
}