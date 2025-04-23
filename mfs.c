/**************************************************************
* Class::  CSC-415-02 Spring 2025
* Name:: Nabeel Rana, Leigh Ann Apotheker, Ethan Zheng, Bryan Mendez
* Student IDs:: 924432311, 923514173, 922474550, 922744724
* GitHub-Name:: nabware
* Group-Name:: Team 42
* Project:: Basic File System
*
* File:: mfs.c
*
* Description:: 
*	This is the implementation of the file system interface.
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "fsLow.h"
#include "mfs.h"

VCB *vcb;
int *fatTable;
char cwd[PATHMAX_LEN] = {'/', '\0'};

// Takes a start block number and returns the first free block number.
int findNextFreeBlock(int startBlockNum)
{
    for (int i = startBlockNum + 1; i < vcb->numOfBlocks; i++)
    {
        if (fatTable[i] == FREE_BLOCK_FLAG)
        {
            return i;
        }
    }

    return -1;
}

// Takes a start block number and number of blocks, allocates blocks, and returns
// start block number.
int allocateBlocks(int startBlockNum, int numOfBlocks)
{
	// First check if we have enough free blocks available
	int blockAllocations[numOfBlocks];
	int currentFreeBlock = startBlockNum;
	for (int i = 0; i < numOfBlocks; i++)
	{
	    blockAllocations[i] = currentFreeBlock;
	    int nextFreeBlock = findNextFreeBlock(currentFreeBlock);
	    if (nextFreeBlock == -1)
	    {
	        printf("Failed to allocate blocks.\n");
	        return -1;
	    }
	    currentFreeBlock = nextFreeBlock;
	}

    // Then allocate blocks, pointing each block to the next block in chain
	for (int i = 0; i < numOfBlocks; i++)
	{
	    int blockNum = blockAllocations[i];
	    if (i == numOfBlocks - 1)
	    {
	        fatTable[blockNum] = END_OF_FILE_FLAG;
	    }
	    else
	    {
	        int nextBlockNum = blockAllocations[i + 1];
	        fatTable[blockNum] = nextBlockNum;
	    }
	}

	LBAwrite(fatTable, vcb->fatNumOfBlocks, vcb->fatBlockNum);

	// Keep track of the new free block number
	vcb->freeBlockNum = findNextFreeBlock(blockAllocations[numOfBlocks - 1]);
    LBAwrite(vcb, 1, vcb->fsBlockNum);

	return startBlockNum;
}

// Takes a start block number and frees blocks until it reaches an end of file
// block.
void freeBlocks(int startBlock)
{
    if (startBlock < 0)
    {
        return;
    }
    int currBlock = startBlock;
    while (currBlock != END_OF_FILE_FLAG)
    {
        int nextBlock = fatTable[currBlock];
        fatTable[currBlock] = FREE_BLOCK_FLAG;
        currBlock = nextBlock;
    }
    LBAwrite(fatTable, vcb->fatNumOfBlocks, vcb->fatBlockNum);

    // Keep track of the new free block number if it starts earlier than the
    // current free block.
    if (startBlock < vcb->freeBlockNum)
    {
        vcb->freeBlockNum = startBlock;
        LBAwrite(vcb, 1, vcb->fsBlockNum);
    }
}

// Reads blocks of directory entires from disk
void readDirEntries(DE* dirEntries, int startBlock)
{
    // This could be improved by checking for contigious blocks before reading
    int currBlock = startBlock;
    int blockOffset = 0;
    int entriesPerBlock = vcb->blockSize / sizeof(DE);
    while (currBlock != END_OF_FILE_FLAG)
    {
        LBAread(&dirEntries[blockOffset], 1, currBlock);
        currBlock = fatTable[currBlock];
        blockOffset = blockOffset + entriesPerBlock;
    }
}

// Write blocks of directory entires to disk
void writeDirEntries(DE* dirEntries, int startBlock)
{
    // This could be improved by checking for contigious blocks before reading
    int currBlock = startBlock;
    int blockOffset = 0;
    int entriesPerBlock = vcb->blockSize / sizeof(DE);
    while (currBlock != END_OF_FILE_FLAG)
    {
        LBAwrite(&dirEntries[blockOffset], 1, currBlock);
        currBlock = fatTable[currBlock];
        blockOffset = blockOffset + entriesPerBlock;
    }
}

// Takes the path to a directory and returns it's directory entries
DE* getDirEntriesFromPath(const char *pathname)
{
    // Get the root directory entries
    int requiredNumOfBytes = vcb->numOfEntries * sizeof(DE);
    int requiredNumOfBlocks = (requiredNumOfBytes + (vcb->blockSize - 1)) /
                              vcb->blockSize;
    DE* dirEntries = malloc(requiredNumOfBlocks * vcb->blockSize);
    if (dirEntries == NULL)
    {
        return NULL;
    }
    readDirEntries(dirEntries, vcb->rootDirBlockNum);

    // Get the absolute path
    char absolutePath[PATHMAX_LEN];
    getAbsolutePath(absolutePath, pathname);

    // Look for each directory in the path and get their directory entries
    // to then look for the next directory until you're left with the 
    // directory entries of the last directory in the path.
    char* dirName = strtok(absolutePath, "/");
    while (dirName != NULL)
    {
        int dirFound = 0;
        for (int i = 0; i < vcb->numOfEntries; i++)
        {
            if (dirEntries[i].used == 1 && dirEntries[i].isDirectory == 1)
            {
                // When we find the directory, get that directory's
                // directory entries.
                if (strcmp(dirName, dirEntries[i].name) == 0)
                {
                    dirFound = 1;
                    readDirEntries(dirEntries, dirEntries[i].location);
                    break;
                }
            }
        }
        // If we cannot find the directory in the current directory entries then
        // the path is invalid.
        if (dirFound == 0)
        {
            free(dirEntries);
            return NULL;
        }
        dirName = strtok(NULL, "/");
    }

    return dirEntries;
}

// Takes a path to a file or directory and returns it's directory entry.
DE* getDirEntryFromPath(const char *pathname)
{
    // Get absolute path
    char absolutePath[PATHMAX_LEN];
    getAbsolutePath(absolutePath, pathname);

    // Get the directory entries from the parent of last element
    char parentPath[PATHMAX_LEN];
    getParentPath(parentPath, absolutePath);
    DE *dirEntries = getDirEntriesFromPath(parentPath);
    if (dirEntries == NULL)
    {
        return NULL;
    }

    // Find the directory entry of the last element of the path
    char lastElement[PATHMAX_LEN];
    getLastElementName(lastElement, absolutePath);
    DE *dirEntry = malloc(sizeof(DE));
    if (dirEntry == NULL)
    {
        return NULL;
    }
    int dirEntryFound = 0;
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        if (dirEntries[i].used == 1 && strcmp(dirEntries[i].name, lastElement) == 0)
        {
            dirEntryFound = 1;
            memcpy(dirEntry, &dirEntries[i], sizeof(DE));
            break;
        }
    }
    if (dirEntryFound == 0)
    {
        free(dirEntries);
        free(dirEntry);
        return NULL;
    }
    free(dirEntries);

    return dirEntry;
}

fdDir * fs_opendir(const char *pathname)
{
    // Get the directory entries and initialize the file descriptor for the
    // directory.
    DE *directory = getDirEntriesFromPath(pathname);
    if (directory == NULL)
    {
        return NULL;
    }
    fdDir *fd = malloc(sizeof(fdDir));
    fd->dirEntryPosition = 0;
    fd->d_reclen = vcb->numOfEntries;
    fd->directory = directory;
    fd->di = malloc(sizeof(struct fs_diriteminfo));

    return fd;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    // Look for the next used directory entry and initialize the directory item.
    struct fs_diriteminfo * di = NULL;
    for (int i = dirp->dirEntryPosition; i < dirp->d_reclen; i++)
    {
        if (dirp->directory[i].used == 1)
        {
            di = dirp->di;
            di->d_reclen = dirp->directory[i].size;
            strcpy(di->d_name, dirp->directory[i].name);
        }
        dirp->dirEntryPosition = dirp->dirEntryPosition + 1;
        if (di != NULL)
        {
            break;
        }
    }
    return di;
}

int fs_closedir(fdDir *dirp)
{
    if (dirp != NULL)
    {
        if (dirp->directory != NULL)
        {
            free(dirp->directory);
        }
        if (dirp->di != NULL)
        {
            free(dirp->di);
        }
        free(dirp);
    }
    return 0;
}

int fs_stat(const char *path, struct fs_stat *buf)
{
    DE* dirEntry = getDirEntryFromPath(path);
    if (dirEntry == NULL)
    {
        return -1;
    }

    buf->st_size = dirEntry->size;
    buf->st_createtime = dirEntry->createdAt;
    buf->st_accesstime = dirEntry->modifiedAt;
    buf->st_modtime = dirEntry->modifiedAt;

    return 0;
}

// Takes a filename and returns whether or not its a file.
int fs_isFile(const char * filename)
{
    DE* dirEntry = getDirEntryFromPath(filename);
    if (dirEntry == NULL)
    {
        return 0;
    }
    int isFile = dirEntry->isDirectory == 0 ? 1 : 0;
    free(dirEntry);
    return isFile;
}

// Takes a pathname and returns whether or not its a directory.
int fs_isDir(const char * pathname)
{
    DE* dirEntry = getDirEntryFromPath(pathname);
    if (dirEntry == NULL)
    {
        return 0;
    }
    int isDir = dirEntry->isDirectory;
    free(dirEntry);
    return isDir;
}

char * fs_getcwd(char *pathname, size_t size)
{
    strncpy(pathname, cwd, size);
    return pathname;
}

int fs_setcwd(char *pathname)
{
    // Get absolute path
    char absolutePath[PATHMAX_LEN];
    getAbsolutePath(absolutePath, pathname);

    // Check if path exists
    int isDir = fs_isDir(absolutePath);
    if (isDir == 0)
    {
        return -1;
    }

    strncpy(cwd, absolutePath, PATHMAX_LEN);

    return 0;
}

int fs_create(char* filename)
{
    int isDir = fs_isDir(filename);
	int isFile = fs_isFile(filename);
    if (isDir == 1 || isFile == 1)
    {
        return -1;
    }

    // Get absolute path
    char absolutePath[PATHMAX_LEN];
    getAbsolutePath(absolutePath, filename);

    // Get the parent path of last element
    char parentPath[PATHMAX_LEN];
    getParentPath(parentPath, absolutePath);

    // Look for an unsed directory entry in parent directory
    DE *dirEntries = getDirEntriesFromPath(parentPath);
    if (dirEntries == NULL)
    {
        return -1;
    }
    int dirEntryIndex = -1;
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        if (dirEntries[i].used == 0)
        {
            dirEntryIndex = i;
            break;
        }
    }
    // No free directory entries available
    if (dirEntryIndex == -1)
    {
        free(dirEntries);
        return -1;
    }

    // Initialize directory entry
    int startBlock = allocateBlocks(vcb->freeBlockNum, 1);
    char fileName[PATHMAX_LEN];
    getLastElementName(fileName, absolutePath);
    strcpy(dirEntries[dirEntryIndex].name, fileName);
    time_t timeNow;
    time(&timeNow);
    dirEntries[dirEntryIndex].used = 1;
    dirEntries[dirEntryIndex].location = startBlock;
    dirEntries[dirEntryIndex].size = 0;
    dirEntries[dirEntryIndex].isDirectory = 0;
    dirEntries[dirEntryIndex].createdAt = timeNow;
    dirEntries[dirEntryIndex].modifiedAt = timeNow;

    // Write directory entries to disk
    DE *parentDirEntry = getDirEntryFromPath(parentPath);
    if (parentDirEntry == NULL)
    {
        free(dirEntries);
        return -1;
    }
    writeDirEntries(dirEntries, parentDirEntry->location);

    free(dirEntries);
    free(parentDirEntry);
    return 0;
}

int fs_delete(char* filename)
{
    // Update parent directory entries and free blocks
    char absolutePath[PATHMAX_LEN];
    getAbsolutePath(absolutePath, filename);
    char parentPath[PATHMAX_LEN];
    getParentPath(parentPath, absolutePath);
    char fileName[PATHMAX_LEN];
    getLastElementName(fileName, absolutePath);
    DE *parentDirEntry = getDirEntryFromPath(parentPath);
    if (parentDirEntry == NULL)
    {
        return -1;
    }
    DE *parentDirEntries = getDirEntriesFromPath(parentPath);
    if (parentDirEntries == NULL)
    {
        free(parentDirEntry);
        return -1;
    }
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        if (parentDirEntries[i].used == 1 &&
            strcmp(parentDirEntries[i].name, fileName) == 0)
        {
            parentDirEntries[i].used = 0;
            freeBlocks(parentDirEntries[i].location);
            break;
        }
    }
    writeDirEntries(parentDirEntries, parentDirEntry->location);
    free(parentDirEntry);
    free(parentDirEntries);

    return 0;
}

int fs_move(char* src, char* dest)
{
    // Check that source is a file, destination is not a file/dir, and the
    // destinations parent is a directory.
    int srcIsFile = fs_isFile(src);
    if (srcIsFile == 0)
    {
        return -1;
    }
    int destIsFile = fs_isFile(dest);
    int destIsDir = fs_isDir(dest);
    if (destIsFile == 1 || destIsDir == 1)
    {
        return -1;
    }
    char destAbsolutePath[PATHMAX_LEN];
    getAbsolutePath(destAbsolutePath, dest);
    char destParentPath[PATHMAX_LEN];
    getParentPath(destParentPath, destAbsolutePath);
    int destParentIsDir = fs_isDir(destParentPath);
    if (destParentIsDir == 0)
    {
        return -1;
    }    

    // Get unused directory entry in destination parent directory
    DE* destParentDirEntries = getDirEntriesFromPath(destParentPath);
    int destDirEntryIndex = -1;
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        if (destParentDirEntries[i].used == 0)
        {
            destDirEntryIndex = i;
            break;
        }
    }
    // No directory entries available
    if (destDirEntryIndex == -1)
    {
        free(destParentDirEntries);
        return -1;
    }

    // Copy source directory entry to destination directory entry,
    // set destination filename, and mark source directory entry as unused.
    char destFilename[PATHMAX_LEN];
    getLastElementName(destFilename, destAbsolutePath);
    char srcAbsolutePath[PATHMAX_LEN];
    getAbsolutePath(srcAbsolutePath, src);
    char srcParentPath[PATHMAX_LEN];
    getParentPath(srcParentPath, srcAbsolutePath);
    char srcFilename[PATHMAX_LEN];
    getLastElementName(srcFilename, srcAbsolutePath);
    DE* srcParentDirEntries = getDirEntriesFromPath(srcParentPath);
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        if (srcParentDirEntries[i].used == 1 &&
            strcmp(srcParentDirEntries[i].name, srcFilename) == 0)
        {
            memcpy(&destParentDirEntries[destDirEntryIndex],
                   &srcParentDirEntries[i],
                   sizeof(DE));
            strcpy(destParentDirEntries[destDirEntryIndex].name, destFilename);
            srcParentDirEntries[i].used = 0;
            // If the src and dest are the same, you need to set the dest dir
            // entry's used field to 0.
            if (strcmp(srcParentPath, destParentPath) == 0)
            {
                destParentDirEntries[i].used = 0;
            }
            break;
        }
    }
    // Write to disk
    DE* srcParentDirEntry = getDirEntryFromPath(srcParentPath);
    DE* destParentDirEntry = getDirEntryFromPath(destParentPath);
    writeDirEntries(srcParentDirEntries, srcParentDirEntry->location);
    writeDirEntries(destParentDirEntries, destParentDirEntry->location);
    free(srcParentDirEntry);
    free(destParentDirEntry);
    free(srcParentDirEntries);
    free(destParentDirEntries);

    return 0;
}

// Takes a pathname which could be absolute or relative
// and writes absolute path to buffer.
void getAbsolutePath(char* absolutePath, const char *pathname)
{
    // If the pathname starts with '/' we're given an absolute path, otherwise
    // it's a relative path and we need to start with the current working
    // directory and add the pathname to it.
    char tempPath[PATHMAX_LEN];
    if (pathname[0] == '/')
    {
        strncpy(tempPath, pathname, PATHMAX_LEN);
    }
    else
    {
        strncpy(tempPath, cwd, PATHMAX_LEN);
        strcat(tempPath, "/");
        strcat(tempPath, pathname);
    }

    // Build an array of the path elements to handle cases where the path
    // includes '.' and '..'.
    char* pathElements[PATHMAX_LEN];
    int pathElementCount = 0;
    char *pathElement = strtok(tempPath, "/");
    while (pathElement != NULL)
    {
        if (strcmp(pathElement, "..") == 0)
        {
            if (pathElementCount > 0)
            {
                pathElementCount = pathElementCount - 1;
            }
        }
        else if (strcmp(pathElement, ".") != 0)
        {
            pathElements[pathElementCount] = pathElement;
            pathElementCount = pathElementCount + 1;
        }
        pathElement = strtok(NULL, "/");
    }

    // Build the absolute path, placing "/" between the path elements
    strncpy(absolutePath, "/", PATHMAX_LEN);
    for (int i = 0; i < pathElementCount; i++)
    {
        strcat(absolutePath, pathElements[i]);
        if (i < pathElementCount - 1)
        {
            strcat(absolutePath, "/");
        }
    }
}

// Takes a path, finds the last element, and copies it to buffer
void getLastElementName(char * elementName, char * path)
{
    char pathCopy[PATHMAX_LEN];
    strcpy(pathCopy, path);
    // Build an array of the elements of the path
    char *pathElements[PATHMAX_LEN];
    int pathElementCount = 0;
    char *pathElement = strtok(pathCopy, "/");
    while (pathElement != NULL)
    {
        pathElements[pathElementCount] = pathElement;
        pathElementCount = pathElementCount + 1;
        pathElement = strtok(NULL, "/");
    }
    // If the path was just "/" then use "." as the only path element
    if (pathElementCount == 0)
    {
        pathElements[pathElementCount] = ".";
        pathElementCount = pathElementCount + 1;
    }
    strcpy(elementName, pathElements[pathElementCount - 1]);
}

// Takes a path, finds the path to the parent of the last element and,
// writes it to a buffer.
void getParentPath(char *parentPath, char *path)
{
    char pathCopy[PATHMAX_LEN];
    strcpy(pathCopy, path);
    // Build an array of the elements of the path
    char *pathElements[PATHMAX_LEN];
    int pathElementCount = 0;
    char *pathElement = strtok(pathCopy, "/");
    while (pathElement != NULL)
    {
        pathElements[pathElementCount] = pathElement;
        pathElementCount = pathElementCount + 1;
        pathElement = strtok(NULL, "/");
    }
    // Build the parent path, placing "/" between the path elements
    // and skipping the last element.
    strncpy(parentPath, "/", PATHMAX_LEN);
    for (int i = 0; i < pathElementCount - 1; i++)
    {
        strcat(parentPath, pathElements[i]);
        if (i < pathElementCount - 2)
        {
            strcat(parentPath, "/");
        }
    }
}

int fs_mkdir(const char *pathname, mode_t mode)
{
    // Check if directory already exists
    int isDir = fs_isDir(pathname);
    int isFile = fs_isFile(pathname);
    if (isDir == 1 || isFile == 1)
    {
        printf("cannot create directory '%s': File exists\n", pathname);
        return -1;
    }

    // Get absolute path
    char absolutePath[PATHMAX_LEN];
    getAbsolutePath(absolutePath, pathname);

    // Get the parent path of last element
    char parentPath[PATHMAX_LEN];
    getParentPath(parentPath, absolutePath);

    // Look for an unsed directory entry in parent directory
    DE *parentDirEntries = getDirEntriesFromPath(parentPath);
    int dirEntryIndex = -1;
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        if (parentDirEntries[i].used == 0)
        {
            dirEntryIndex = i;
            break;
        }
    }
    // No free directory entries available
    if (dirEntryIndex == -1)
    {
        free(parentDirEntries);
        return -1;
    }

    // Initialize directory entry
    int requiredNumOfBytes = vcb->numOfEntries * sizeof(DE);
    int requiredNumOfBlocks = (requiredNumOfBytes + (vcb->blockSize - 1)) /
                              vcb->blockSize;
    int childDirStartBlock = allocateBlocks(vcb->freeBlockNum, requiredNumOfBlocks);
    char lastElement[PATHMAX_LEN];
    getLastElementName(lastElement, absolutePath);
    strcpy(parentDirEntries[dirEntryIndex].name, lastElement);
    time_t timeNow;
    time(&timeNow);
    parentDirEntries[dirEntryIndex].used = 1;
    parentDirEntries[dirEntryIndex].location = childDirStartBlock;
    parentDirEntries[dirEntryIndex].size = 0;
    parentDirEntries[dirEntryIndex].isDirectory = 1;
    parentDirEntries[dirEntryIndex].createdAt = timeNow;
    parentDirEntries[dirEntryIndex].modifiedAt = timeNow;

    // Get parent dir entry to create ".." directory in new dir entries and to
    // save updated parent dir entries to disk.
    DE *parentDirEntry = getDirEntryFromPath(parentPath);
    if (parentDirEntry == NULL)
    {
        free(parentDirEntries);
        return -1;
    }

    // Initialize directory entries for the new directory
    DE *childDirEntries = malloc(requiredNumOfBytes);
    if (childDirEntries == NULL)
    {
        printf("Error allocating memory for directory entries.\n");
        free(parentDirEntry);
        free(parentDirEntries);
		return -1;
    }
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        childDirEntries[i].used = 0;
    }

    // Initialize the two directories "." and ".."
    memcpy(&childDirEntries[1], parentDirEntry, sizeof(DE));
    strcpy(childDirEntries[1].name, "..");
    strcpy(childDirEntries[0].name, ".");
    childDirEntries[0].location = childDirStartBlock;
    childDirEntries[0].used = 1;
    childDirEntries[0].size = 0;
    childDirEntries[0].isDirectory = 1;
    childDirEntries[0].createdAt = timeNow;
    childDirEntries[0].modifiedAt = timeNow;

    // Write to disk at end to ensure no errors have occured up until this point.
    writeDirEntries(parentDirEntries, parentDirEntry->location);
    writeDirEntries(childDirEntries, childDirStartBlock);
    free(parentDirEntries);
    free(childDirEntries);
    free(parentDirEntry);

    return 0;
}


int fs_rmdir(const char *pathname)
{
    // Make sure directory is empty
    DE *dirEntries = getDirEntriesFromPath(pathname);
    if (dirEntries == NULL)
    {
        return -1;
    }
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        if (dirEntries[i].used == 1 && strcmp(dirEntries[i].name, ".") != 0
                                    && strcmp(dirEntries[i].name, "..") != 0)
        {
            printf("Cannot remove '%s': Directory is not empty.\n", pathname);
            free(dirEntries);
            return -1;
        }
    }
    free(dirEntries);

    // Update parent directory entries and free blocks
    char absolutePath[PATHMAX_LEN];
    getAbsolutePath(absolutePath, pathname);
    char parentPath[PATHMAX_LEN];
    getParentPath(parentPath, absolutePath);
    char dirName[PATHMAX_LEN];
    getLastElementName(dirName, absolutePath);
    DE *parentDirEntry = getDirEntryFromPath(parentPath);
    if (parentDirEntry == NULL)
    {
        return -1;
    }
    DE *parentDirEntries = getDirEntriesFromPath(parentPath);
    if (parentDirEntries == NULL)
    {
        free(parentDirEntry);
        return -1;
    }
    for (int i = 0; i < vcb->numOfEntries; i++)
    {
        if (parentDirEntries[i].used == 1 &&
            strcmp(parentDirEntries[i].name, dirName) == 0)
        {
            parentDirEntries[i].used = 0;
            freeBlocks(parentDirEntries[i].location);
            break;
        }
    }
    writeDirEntries(parentDirEntries, parentDirEntry->location);
    free(parentDirEntry);
    free(parentDirEntries);

    return 0;
}
