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

	char arr1[] = "testing/test.txt";
	if (File_Create(arr1) == -1)
	{
		printf("Error During File_Create\n");
		exit(1);
	}

	char arr2[] = "testing/test.txt";
	int fd = File_Open(arr2);
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

	char arr3[] = "testing/testing2";
	if (Dir_Create(arr3) == -1)
	{
		printf("Error During Dir_Create\n");
		exit(1);
	}

	char arr4[] = "testing/testing2/test.txt";
	if (File_Create(arr4) == -1)
	{
		printf("Error During File_Create\n");
		exit(1);
	}

	if (FS_Sync() == -1)
	{
		printf("Error During FS_Sync\n");
		exit(1);
	}
}