#include <stdio.h>
#include "LibFS.h"

int main()
{
	char path[15] = "FS.txt";
	char arr[10][SECTOR_SIZE] = {"/folder1", "/folder1/file1.txt", "Hi! My name is Narayan", "/folder1/file2.txt", "Text in file2"};

	printf("DONT USE TRAILING '/' IN THE PATH NAMES\n");
	printf("%d\n", FS_Boot(path));

	printf("%d\n", Dir_Create(arr[0]));

	printf("%d\n", File_Create(arr[1]));
	int fd = File_Open(arr[1]);
	printf("fd=%d\n", fd);
	printf("%d\n", File_Write(fd, arr[2], 10));
	printf("%d\n", File_Seek(fd, 4));
	printf("%d\n", File_Read(fd, arr[9], 3));
	printf("String read:%s\n", arr[9]);
	printf("%d\n", File_Read(fd, arr[9], 3));
	printf("String read:%s\n", arr[9]);
	printf("%d\n", File_Seek(2, 0));

	printf("%d\n", File_Create(arr[3]));
	int fd2 = File_Open(arr[3]);
	printf("fd2=%d\n", fd2);
	printf("%d\n", File_Write(fd2, arr[4], 10));
	printf("%d\n", File_Seek(fd2, 2));
	printf("%d\n", File_Read(fd2, arr[9], 100));
	printf("String read:%s\n", arr[9]);

	printf("%d\n", Dir_Read(arr[0], arr[9], 100));
	printf("Files in the directory: %s\n", arr[9]);

	printf("%d\n", File_Unlink(arr[1]));
	printf("%d\n", File_Close(fd));
	printf("%d\n", File_Unlink(arr[1]));

	printf("%d\n", Dir_Unlink(arr[0]));
	printf("%d\n", File_Close(fd2));
	printf("%d\n", File_Unlink(arr[3]));
	printf("%d\n", Dir_Unlink(arr[0]));

	FS_Sync();
	return 0;
}