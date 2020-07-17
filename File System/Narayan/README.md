


# File System

## Generic APIs

----
1. int FS_Boot(char *path)
    Should be called exactly once before any other LibFS functions are called. It takes a single argument, the path, which either points to a real file where your "disk image" is stored or to a file that does not yet exist and which must be created to hold a new disk image. Upon success, returns 0. Upon failure, returns -1 and sets osErrno to E_GENERAL.

2. int FS_Sync()
    It makes sure the contents of the file system are stored persistently on disk. Upon success, returns 0. Upon failure, returns -1 and sets osErrno to E_GENERAL.


## File Access APIs

----
1. int File_Create(char *file)

    Creates a new file of the name pointed to by file. If the file directory does not exist, returns -1 and sets osErrno to E_CREATE. 

2. int File_Open(char *file)

    Opens up a file (whose name is pointed to by file ) and returns an integer file descriptor (a number greater than or equal to 0), which can be used to read or write data to that file. If the file doesn't exist, returns -1 and sets osErrno should be set to E_NO_SUCH_FILE. If there are already a maximum number of files open, returns -1 and sets osErrno to E_TOO_MANY_OPEN_FILES.

3. int File_Read(int fd, void *buffer, int size)

    Reads size bytes from the file referenced by the file descriptor fd. The data is read into the buffer. All reads begin at the current location of the file pointer, and file pointer is updated after the read to the new location. If the file is not open, returns -1, and sets osErrno to E_BAD_FD. If the file is open, the number of bytes actually read is returned, which can be less than or equal to size. 

4. int File_Write(int fd, void *buffer, int size)

    Writes size bytes from buffer into the file referenced by fd. All writes begin at the current location of the file pointer and the file pointer is updated after the write to its current location plus size. If the file is not open, returns -1 and sets osErrno to E_BAD_FD.

5. int File_Seek(int fd, int offset)

    Updates the current location of the file pointer. The location is given as an offset from the beginning of the file. If offset is larger than the size of the file or negative, returns -1 and sets osErrno to E_SEEK_OUT_OF_BOUNDS. If the file is not currently open, returns -1 and sets osErrno to E_BAD_FD. Upon success, returns the new location of the file pointer.

6. int File_Close(int fd)

    Closes the file referred to by file descriptor fd. If the file is not currently open, returns -1 and sets osErrno to E_BAD_FD. Upon success, returns 0.

7. int File_Unlink(char *file)
    Deletes the file referenced by file, including removing its name from the directory it is in, and freeing up any data blocks and inodes that the file was using. If the file does not currently exist, returns -1 and sets osErrno to E_NO_SUCH_FILE. If the file is currently open, returns -1 and sets osErrno to E_FILE_IN_USE (and do NOT delete the file). Upon success, returns 0.

## Directory APIs

----
1. int Dir_Create(char *path)

    Creates a new directory as named by path. All paths are absolute paths. Upon failure of any sort, returns -1 and set osErrno to E_CREATE. Upon success, returns 0. Note that Dir_Create() is not recursive -- that is, if only "/" exists, and you want to create a directory "/a/b/", you must first create "/a", and then create "/a/b".

2. int Dir_Read(char *path, void *buffer, int size)

    Reads the contents of a directory. Returns in the buffer a set of directory entries. Each entry is of size 20 bytes, and contains 16-byte names of the directories and files within the directory named by path, followed by the 4-byte integer inode number. If size is not big enough to contain all of the entries, returns -1 and set osErrno to E_BUFFER_TOO_SMALL. Otherwise, read the data into the buffer, and return the number of directory entries that are in the directory.

3. int Dir_Unlink(char *path)

    Removes a directory referred to by path, freeing up its inode and data blocks, and removing its entry from the parent directory. Upon success, returns 0. Note: Dir_Unlink() is only successful if there are no files within the directory. If there are still files within the directory, it returns -1 and sets osErrno to E_DIR_NOT_EMPTY. It the directory referred is the root directory, returns -1 and sets osErrno to E_ROOT_DIR.


## How to run main.c

----
    $ gcc LibDisk.c Helpers.c LibFS.c main.c
    $ ./a.out

