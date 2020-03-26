#include <stdio.h>
#include <stdlib.h>
#include "LibFS.h"

int main()
{
	if (FS_Boot() == -1)
	{
		printf("Error During FS_Boot\n");
		exit(1);
	}

	if (Dir_Create("testing") == -1)
	{
		printf("Error During Dir_Create\n");
		exit(1);
	}

	char arr[] = "testing/test.txt";
	if (File_Create(arr) == -1)
	{
		printf("Error During File_Create\n");
		exit(1);
	}

	int fd = File_Open(arr);
	if (fd == -1)
	{
		printf("Error During File_Open\n");
		exit(1);
	}

	if (File_Write(fd, "Test", 5) == -1)
	{
		printf("Error During File_Write\n");
		exit(1);
	}

	if (File_Close(fd) == -1)
	{
		printf("Error During File_Close\n");
		exit(1);
	}

	if (FS_Sync() == -1)
	{
		printf("Error During FS_Sync\n");
		exit(1);
	}
}