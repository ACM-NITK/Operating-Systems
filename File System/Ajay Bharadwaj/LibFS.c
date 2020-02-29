#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "LibFS.h"
#include "LibDisk.h"

//Maximum number (Arbitrary) of Directories (Including the Root Directory) and Files in the Entire File System
#define MAXDIR 100
#define MAXFILE 100

//Maximum number (Arbitrary) of Directories and Files in any Directory
#define MAXDIRDIR 10
#define MAXDIRFILE 10

//Maximum length (Arbitrary) of a Directory or File Name
#define NAMELEN 16

//Maximum number (Arbitrary) of Sectors that can be allocated to a File (Places an Upper Bound on the size of the File)
#define MAXSECTORPERFILE 10

//Maximum number (Arbitrary) of Files that can be open at the same time
#define MAXOPENFILES 5

#define FILELOC "./diskFile.bin"

// Global error number
int osErrno;

/*
Magic Number (0xBADA55AF) - int

Number of Directories - MAXDIR
	Each Directory Node - NAMELEN*char + int + MAXDIRDIR*int + int + MAXDIRFILE*int + int

Number of Files - MAXFILE
	Each File Node - int + NAMELEN*char + int + int + MAXSECTORPERFILE*int

Sectors
	int + NUM_SECTORS*int
*/
void* fsBuffer = NULL;
int totalData = (NUM_SECTORS + 4)*sizeof(int) + MAXDIR*(NAMELEN*sizeof(char) + 3*sizeof(int) + (MAXDIRDIR+MAXDIRFILE)*sizeof(int)) + MAXFILE*(NAMELEN*sizeof(char) + 3*sizeof(int) + MAXSECTORPERFILE*sizeof(int));
int fsSectors = -1;

int numDir;
struct dirNode
{
	char name[NAMELEN];
	
	int numChildDir;
	int childDir[MAXDIRDIR];
	
	int numChildFile;
	int childFile[MAXDIRFILE];
	
	int free;
} *dir;

int numFile;
struct fileNode
{
	int size;
	char name[NAMELEN];
	int free;
	
	int numFileSectors;
	int fileSectors[MAXSECTORPERFILE];
} *file;

int numSectors;
int* sectors;

int numOpenFiles = 0;
struct openFile
{
	void* buffer;
	int ptr;
	int fd;
	int fileInd;
	int size;
	int free;
} openFiles[MAXOPENFILES];

void initializeOpenFiles()
{
	for (int a = 0; a < MAXOPENFILES; a++)
	{
		openFiles[a].free = 1;
	}
}

//Search the Current Directory for a Directory
int getDirInd(int curInd, char* prev)
{
	int dirInd = -1;
	for (int a = 0; a < dir[curInd].numChildDir; a++)
	{
		if (!strcmp(prev, dir[dir[curInd].childDir[a]].name))
		{
			dirInd = a;
			break;
		}
	}
	
	if (dir[dirInd].free)
	{
		return -1;
	}
	
	return dirInd;
}

//Search the Current Directory for a File
int getFileInd(int curInd, char* prev)
{
	int fileInd = -1;
	for (int a = 0; a < dir[curInd].numChildFile; a++)
	{
		if (!strcmp(prev, file[dir[curInd].childFile[a]].name))
		{
			fileInd = a;
			break;
		}
	}
	
	if (file[fileInd].free)
	{
		return -1;
	}
	
	return fileInd;
}

int traverseDownToFile(char* path)
{
	int curInd = 0;
    
    char* next = strtok(path, "/");    
    char* prev = NULL;
    while (next)
    {
    	curInd = getDirInd(curInd, next);
		if (curInd == -1)
    	{
    		prev = next;
    		next = strtok(NULL, "/");
    		
    		if (next != NULL)
    		{
    			return -1;
			}
			else
			{
				break;
			}
		}
		else
		{
			if (dir[curInd].free)
			{
				return -1;
			}
			prev = next;
    		next = strtok(NULL, "/");
		}
	}
	
	return curInd;
}

char* getFileName(char* path)
{
	char* next = strtok(path, "/");    
    char* prev = NULL;
    while (next)
    {
		prev = next;
		next = strtok(NULL, "/");
	}
	
	return prev;
}

int min(int a, int b)
{
	if (a < b)
	{
		return a;
	}
	else
	{
		return b;
	}
}

void setupDisk()
{
	int* temp = (int*) fsBuffer;
	*temp = 0xBADA55AF;
	
	numDir = 1;
	dir[0].free = 0;
	dir[0].numChildDir = 0;
	dir[0].numChildFile = 0;
	strcpy(dir[0].name, "ROOT");
	for (int a = 0; a < MAXDIR; a++)
	{
		dir[a].free = 1;
	}
	
	numFile = 0;
	for (int a = 0; a < MAXFILE; a++)
	{
		file[a].free = 1;
	}
	
	numSectors = 0;
	for (int a = 0; a < NUM_SECTORS; a++)
	{
		sectors[a] = 1;
	}
}

void setVariables()
{
	void* temp = fsBuffer;
	
	temp = (void*) (((int*) temp) + 1);
	numDir = *((int*) temp);
	
	temp = (void*) (((int*) temp) + 1);
	dir = (struct dirNode*) temp;
	
	temp = (void*) (((struct dirNode*) temp) + MAXDIR);
	numFile = *((int*) temp);
	
	temp = (void*) (((int*) temp) + 1);
	file = (struct fileNode*) temp;
	
	temp = (void*) (((struct fileNode*) temp) + MAXFILE);
	numSectors = *((int*) temp);
	
	temp = (void*) (((int*) temp) + 1);
	sectors = (int*) temp;
}

//Checks whether FILELOC exists or not
int check()
{
	FILE* fp = fopen(FILELOC, "rb");
	if (fp)
	{
		fclose(fp);
		return 1;
	}
	else
	{
		return 0;
	}
}

int FS_Boot()
{
    printf("FS_Boot %s\n", FILELOC);

    if (Disk_Init() == -1)
	{
		printf("Disk_Init() failed\n");
		osErrno = E_GENERAL;
		return -1;
    }
    
    fsSectors = totalData / SECTOR_SIZE;
    if (totalData % SECTOR_SIZE)
    {
    	fsSectors++;
	}
	fsBuffer = malloc(fsSectors*SECTOR_SIZE);
	setVariables();
	
	//To check whether FILELOC exists or not
	if (!check())
	{
		setupDisk();
		return 0;
	}
    
    Disk_Load(FILELOC);
    
    Disk_Read(0, fsBuffer);
    if (*((int*) fsBuffer) == 0xBADA55AF)
    {
    	for (int a = 1; a < fsSectors; a++)
    	{
    		Disk_Read(a, fsBuffer + a*SECTOR_SIZE);
		}
	}
	else
	{
		return -1;
	}
    
    return 0;
}

int FS_Sync()
{
    printf("FS_Sync\n");
    
    for (int a = 0; a < numOpenFiles; a++)
    {
    	if (!openFiles[a].free)
    	{
    		osErrno = E_GENERAL;
    		return -1;
		}
	}
	
	for (int a = 0; a < fsSectors; a++)
	{
		Disk_Write(a, fsBuffer + a*SECTOR_SIZE);
	}
	
	free(fsBuffer);
	
	if (Disk_Save(FILELOC) == -1)
	{
		osErrno = E_GENERAL;
		return -1;
	}
    
    return 0;
}

int File_Create(char *path)
{
    printf("FS_Create\n");
    
    if (numFile >= MAXFILE)
	{
		osErrno = E_CREATE;
    	return -1;
	}
    	
    int curInd = traverseDownToFile(path);
    if (curInd == -1)
    {
    	osErrno = E_CREATE;
    	return -1;
	}
    char* prev = getFileName(path);
	if (dir[curInd].numChildFile >= MAXDIRFILE)
	{
		osErrno = E_CREATE;
    	return -1;
	}
	if (getFileInd(curInd, prev) != -1)
	{
		osErrno = E_CREATE;
   		return -1;
	}
	
	int fileInd;
	for (int a = 0; a < MAXFILE; a++)
	{
		if (file[a].free)
		{
			file[a].free = 0;
			fileInd = a;
			break;
		}
	}
	
	numFile++;
	dir[curInd].childFile[dir[curInd].numChildFile++] = fileInd;
	file[fileInd].size = 0;
	strcpy(file[fileInd].name, prev);
	file[fileInd].numFileSectors = 0;
	
    return 0;
}

int File_Open(char *path)
{
    printf("FS_Open\n");
    
    if (numOpenFiles >= MAXOPENFILES)
    {
    	osErrno = E_TOO_MANY_OPEN_FILES;
    	return -1;
	}
    
    int curInd = traverseDownToFile(path);
    if (curInd == -1)
    {
    	osErrno = E_NO_SUCH_FILE;
    	return -1;
	}
    char* prev = getFileName(path);
	int fileInd = getFileInd(curInd, prev);
	if (fileInd == -1)
	{
		osErrno = E_NO_SUCH_FILE;
		return -1;
	}
	
	numOpenFiles++;
	int fd;
	for (int a = 0; a < MAXOPENFILES; a++)
	{
		if (openFiles[a].free)
		{
			fd = a;
			break;
		}
	}
	
	openFiles[fd].fd = fd;
	openFiles[fd].size = file[fileInd].size;
	openFiles[fd].ptr = 0;
	openFiles[fd].fileInd = fileInd;
	openFiles[fd].buffer = malloc(file[fileInd].numFileSectors*SECTOR_SIZE);
		
	for (int a = 0; a < file[fileInd].numFileSectors; a++)
	{
		Disk_Read(file[fileInd].fileSectors[a], openFiles[fd].buffer + a*SECTOR_SIZE);
	}
	
	return fd;
}

int File_Read(int fd, void *buffer, int size)
{
    printf("FS_Read\n");
    
    if (fd >= numOpenFiles)
    {
    	osErrno = E_BAD_FD;
    	return -1;
	}
	
	if (openFiles[fd].free)
	{
		osErrno = E_BAD_FD;
    	return -1;
	}
	
	int prev = openFiles[fd].ptr;
	openFiles[fd].ptr = min(openFiles[fd].ptr+size, openFiles[fd].size);
	
	for (int a = prev; a <= openFiles[fd].ptr; a++)
	{
		*((char*) (buffer+a)) = *((char*) (openFiles[fd].buffer+a));
	}
    
    return openFiles[fd].ptr - prev;
}

int File_Write(int fd, void *buffer, int size) 
{
    printf("FS_Write\n");
    
    if (fd >= numOpenFiles)
    {
    	osErrno = E_BAD_FD;
    	return -1;
	}
	
	if (openFiles[fd].free)
	{
		osErrno = E_BAD_FD;
    	return -1;
	}
	
	int available = file[openFiles[fd].fileInd].numFileSectors*SECTOR_SIZE - openFiles[fd].ptr;
	int prev = openFiles[fd].ptr;
	
	if (size <= available)
	{
		openFiles[fd].ptr += size;
		for (int a = prev; a <= openFiles[fd].ptr; a++)
		{
			*((char*) (openFiles[fd].buffer+a)) = *((char*) (buffer+a));
		}
	}
	else
	{
		int sectorsReq = (size-available) / SECTOR_SIZE;
		if ((size-available) % SECTOR_SIZE)
		{
			sectorsReq++;
		}
		
		if ((numSectors + sectorsReq) > NUM_SECTORS)
		{
			osErrno = E_NO_SPACE;
			return -1;
		}
		
		if ((file[openFiles[fd].fileInd].numFileSectors + sectorsReq) > MAXSECTORPERFILE)
		{
			osErrno = E_FILE_TOO_BIG;
			return -1;
		}
		
		for (int a = 0; a < NUM_SECTORS && sectorsReq; a++)
		{
			if (sectors[a])
			{
				sectors[a] = 0;
				file[openFiles[fd].fileInd].fileSectors[file[openFiles[fd].fileInd].numFileSectors++] = a;
				sectorsReq--;
			}
		}
		void* newBuffer = malloc(file[openFiles[fd].fileInd].numFileSectors*SECTOR_SIZE);
		
		openFiles[fd].ptr += size;
		for (int a = 0; a <= prev; a++)
		{
			*((char*) (newBuffer+a)) = *((char*) (openFiles[fd].buffer+a));
		}		
		for (int a = prev; a <= openFiles[fd].ptr; a++)
		{
			*((char*) (newBuffer+a)) = *((char*) (buffer+a));
		}
		
		free(openFiles[fd].buffer);
		openFiles[fd].buffer = newBuffer;
	}
    
    return size;
}

int File_Seek(int fd, int offset)
{
    printf("FS_Seek\n");
    
    if (fd >= numOpenFiles)
    {
    	osErrno = E_BAD_FD;
    	return -1;
	}
	
	if (openFiles[fd].free)
	{
		osErrno = E_BAD_FD;
    	return -1;
	}
	
	if (offset < 0 || offset > openFiles[fd].size)
	{
		osErrno = E_SEEK_OUT_OF_BOUNDS;
		return -1;
	}
	
	openFiles[fd].ptr = offset;
    
    return openFiles[fd].ptr;
}

int File_Close(int fd)
{
    printf("FS_Close\n");
    
    if (fd >= numOpenFiles)
    {
    	osErrno = E_BAD_FD;
    	return -1;
	}
	
	if (openFiles[fd].free)
	{
		osErrno = E_BAD_FD;
    	return -1;
	}
	
	int fileInd = openFiles[fd].fileInd;
	for (int a = 0; a < file[fileInd].numFileSectors; a++)
	{
		Disk_Write(file[fileInd].fileSectors[a], openFiles[fd].buffer + a*SECTOR_SIZE);
	}
	
	free(openFiles[fd].buffer);
	openFiles[fd].free = 1;
	numOpenFiles--;
    
    return 0;
}

int File_Unlink(char *path)
{
    printf("FS_Unlink\n");
    
    int curInd = traverseDownToFile(path);
    if (curInd == -1)
    {
    	osErrno = E_NO_SUCH_FILE;
    	return -1;
	}
    char* prev = getFileName(path);
	int fileInd = getFileInd(curInd, prev);
	if (fileInd == -1)
	{
		osErrno = E_NO_SUCH_FILE;
		return -1;
	}
	
	for (int a = 0; a < numOpenFiles; a++)
	{
		if (openFiles[a].fileInd == fileInd && !openFiles[a].free)
		{
			osErrno = E_FILE_IN_USE;
			return -1;
		}
	}
    
    for (int a = 0; a < file[fileInd].numFileSectors; a++)
    {
    	sectors[file[fileInd].fileSectors[a]] = 1;
    	numSectors--;
	}
	
	file[fileInd].free = 1;
	numFile--;
    
    return 0;
}

int Dir_Create(char *path)
{
    printf("Dir_Create %s\n", path);
    
    if (numDir >= MAXDIR)
	{
		osErrno = E_CREATE;
    	return -1;
	}
    
    int curInd = traverseDownToFile(path);
    if (curInd == -1)
    {
    	osErrno = E_CREATE;
    	return -1;
	}
    char* prev = getFileName(path);
    if (!strcmp(dir[curInd].name, prev))
    {
    	osErrno = E_CREATE;
    	return -1;	
	}
	
	int dirInd;
	for (int a = 0; a < MAXDIR; a++)
	{
		if (dir[a].free)
		{
			dir[a].free = 0;
			dirInd = a;
			break;
		}
	}
	
	numDir++;
	dir[curInd].childDir[dir[curInd].numChildDir++] = dirInd;
	strcpy(dir[dirInd].name, prev);
	
	dir[dirInd].numChildDir = 0;
	dir[dirInd].numChildFile = 0;
    
    return 0;
}

int Dir_Size_Rec(int curInd)
{
    int size = 0;
    
    for (int a = 0; a < dir[curInd].numChildDir; a++)
    {
    	size += Dir_Size_Rec(dir[curInd].childDir[a]);
	}
	
	for (int a = 0; a < dir[curInd].numChildFile; a++)
    {
    	size += file[dir[curInd].childFile[a]].size;
	}
	
	return size;
}

int Dir_Size(char *path)
{
    printf("Dir_Size\n");
    
    int curInd = traverseDownToFile(path);
    if (curInd == -1)
    {
    	return -1;
	}
	char* prev = getFileName(path);
	if (strcmp(dir[curInd].name, prev))
    {
    	return -1;	
	}
	
    return Dir_Size_Rec(curInd);
}

int Dir_Read(char *path)
{
    printf("Dir_Read\n");
        
    int curInd = traverseDownToFile(path);
    if (curInd == -1)
    {
    	return -1;
	}
	char* prev = getFileName(path);
	if (strcmp(dir[curInd].name, prev))
    {
    	return -1;	
	}
	
	for (int a = 0; a < dir[curInd].numChildDir; a++)
	{
		printf("%d. %s %d\n", a+1, dir[dir[curInd].childDir[a]].name, dir[curInd].childDir[a]);
	}
    
    return (dir[curInd].numChildDir + dir[curInd].numChildFile);
}

int Dir_Unlink(char *path)
{
    printf("Dir_Unlink\n");
    
    if (!strcmp("/", path))
    {
    	osErrno = E_ROOT_DIR;
    	return -1;
	}
	
	int curInd = traverseDownToFile(path);
    if (curInd == -1)
    {
    	return -1;
	}
	char* prev = getFileName(path);
	if (strcmp(dir[curInd].name, prev))
    {
    	return -1;	
	}
	
	if (dir[curInd].numChildDir || dir[curInd].numChildFile)
	{
		osErrno = E_DIR_NOT_EMPTY;
    	return -1;
	}
	
	dir[curInd].free = 1;
	numDir--;
    
    return 0;
}
