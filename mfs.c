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

int allocateBlocks(int numOfBlocks)
{
	int startBlockNum = vcb->freeBlockNum;

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

	return startBlockNum;
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

// Reads blocks of directory entires from disk
void readDirEntries(DE* dirEntries, int startBlock)
{
    int currBlock = startBlock;
    int blockOffset = 0;
    while (currBlock != END_OF_FILE_FLAG)
    {
        LBAread(&dirEntries[blockOffset], 1, currBlock);
        currBlock = fatTable[currBlock];
        blockOffset = blockOffset + vcb->blockSize;
    }
}

int fs_setcwd(char *pathname) {
    // Check if the pathname is valid
    if (pathname == NULL) {
        return -1;
    }

    //Get absolute path
    char absolutePath[PATHMAX_LEN];
    
    //If it is a relative path, combine it with the current working directory
    
    // Split path into parts
    char pathCopy[PATHMAX_LEN];
    strncpy(pathCopy, absolutePath, PATHMAX_LEN - 1);
    pathCopy[PATHMAX_LEN - 1] = '\0';
    
    char *token;
    char *components[PATHMAX_LEN / 2];
    int numComponents = 0;
    
    token = strtok(pathCopy, "/");
    while (token != NULL && numComponents < PATHMAX_LEN / 2) {
        if (strcmp(token, ".") == 0) {
            // Skip "." entries
        } else if (strcmp(token, "..") == 0) {
            if (numComponents > 0) {
                numComponents--;
            }
        } else {
            components[numComponents++] = token;
        }
        token = strtok(NULL, "/");
    }
    
    //Rebuild clean path

    
    //Check if the directory exists
    DE *dirEntries = getDirEntriesFromPath(cleanPath);
    if (dirEntries == NULL) {
        return -1; 
    }
    
    //Check if it is a directory
    int isDir = 0;
    for (int i = 0; i < vcb->numOfEntries; i++) {
        if (dirEntries[i].used == 1 && 
            strcmp(dirEntries[i].name, ".") == 0 && 
            dirEntries[i].isDirectory == 1) {
            isDir = 1;
            break;
        }
    }
    
    if (!isDir) {
        free(dirEntries);
        return -1; 
    }
    
    // Update the current working directory
    strncpy(cwd, cleanPath, PATHMAX_LEN - 1);
    cwd[PATHMAX_LEN - 1] = '\0';
    
    //Set to root if the current working directory is empty
    if (strlen(cwd) == 0) {
        strcpy(cwd, "/");
    }
    
    if (strcmp(cwd, "/") != 0 && cwd[strlen(cwd) - 1] != '/') {
        strcat(cwd, "/");
    }
    
    free(dirEntries);
    return 0;
}

int fs_mkdir(const char *pathname, mode_t mode) {
    //Check if the pathname is valid
    if (pathname == NULL || strlen(pathname) == 0) {
        return -1;
    }

    // Get the absolute path
    char absolutePath[PATHMAX_LEN];
    getAbsolutePath(absolutePath, pathname);

    //Find the last slash to separate the directory name from the parent path
    char parentPath[PATHMAX_LEN] = {0};
    char dirName[100] = {0};  // Max directory name length is 100 as per DirectoryEntry struct
    
    char *lastSlash = strrchr(absolutePath, '/');
    if (lastSlash == NULL) {
        //Current directory
        strcpy(dirName, absolutePath);
    } else {
        strcpy(dirName, lastSlash + 1);
        if (lastSlash == absolutePath) {
            // The parent is the root directory
            strcpy(parentPath, "/");
        } else {
            strncpy(parentPath, absolutePath, lastSlash - absolutePath);
            parentPath[lastSlash - absolutePath] = '\0';
        }
    }

    if (strlen(dirName) == 0 || strlen(dirName) >= 100) {
        return -1;
    }

    //Get the parent directory entries
    DE *parentDirEntries = getDirEntriesFromPath(parentPath);
    if (parentDirEntries == NULL) {
        return -1;  // Parent directory doesn't exist
    }

    //Check if a directory or file with this name already exists
    for (int i = 0; i < vcb->numOfEntries; i++) {
        if (parentDirEntries[i].used == 1 && strcmp(parentDirEntries[i].name, dirName) == 0) {
            free(parentDirEntries);
            return -1;  
        }
    }

    //Find the first free entry in the parent directory
    int freeEntryIndex = -1;
    for (int i = 0; i < vcb->numOfEntries; i++) {
        if (parentDirEntries[i].used == 0) {
            freeEntryIndex = i;
            break;
        }
    }

    if (freeEntryIndex == -1) {
        free(parentDirEntries);
        return -1;  
    }

    int numOfEntries = vcb->numOfEntries;
    int requiredNumOfBytes = numOfEntries * sizeof(DE);
    int requiredNumOfBlocks = (requiredNumOfBytes + (vcb->blockSize - 1)) / vcb->blockSize;

    //Allocate blocks for the new directory
    int newDirBlockNum = allocateBlocks(requiredNumOfBlocks);
    if (newDirBlockNum == -1) {
        free(parentDirEntries);
        return -1;  // Failed to allocate blocks
    }

    //Initialize the new directory entries
    DE *newDirEntries = malloc(requiredNumOfBlocks * vcb->blockSize);
    if (newDirEntries == NULL) {
        // Free the allocated blocks
        for (int i = 0; i < requiredNumOfBlocks; i++) {
            int blockNum = newDirBlockNum;
            if (i > 0) {
                for (int j = 0; j < i; j++) {
                    blockNum = fatTable[blockNum];
                }
            }
            fatTable[blockNum] = FREE_BLOCK_FLAG;
        }
        LBAwrite(fatTable, vcb->fatNumOfBlocks, vcb->fatBlockNum);
        free(parentDirEntries);
        return -1;
    }

    //Initialize all entries as unused
    for (int i = 0; i < numOfEntries; i++) {
        newDirEntries[i].used = 0;
    }

    int parentDirBlockNum = parentDirEntries[0].location; // "." entry points to the directory itself

    //Set up "." and ".." entries
    time_t timeNow;
    time(&timeNow);

    strcpy(newDirEntries[0].name, ".");
    newDirEntries[0].used = 1;
    newDirEntries[0].location = newDirBlockNum;
    newDirEntries[0].size = 0;
    newDirEntries[0].isDirectory = 1;
    newDirEntries[0].createdAt = timeNow;
    newDirEntries[0].modifiedAt = timeNow;

    strcpy(newDirEntries[1].name, "..");
    newDirEntries[1].used = 1;
    newDirEntries[1].location = parentDirBlockNum;
    newDirEntries[1].size = 0;
    newDirEntries[1].isDirectory = 1;
    newDirEntries[1].createdAt = timeNow;
    newDirEntries[1].modifiedAt = timeNow;

    //Write the new directory entries to disk
    LBAwrite(newDirEntries, requiredNumOfBlocks, newDirBlockNum);
    free(newDirEntries);

    //Update the parent directory entry
    strcpy(parentDirEntries[freeEntryIndex].name, dirName);
    parentDirEntries[freeEntryIndex].used = 1;
    parentDirEntries[freeEntryIndex].location = newDirBlockNum;
    parentDirEntries[freeEntryIndex].size = 0;
    parentDirEntries[freeEntryIndex].isDirectory = 1;
    parentDirEntries[freeEntryIndex].createdAt = timeNow;
    parentDirEntries[freeEntryIndex].modifiedAt = timeNow;

    //Find the parent directory's start block and write the updated entries
    int parentStartBlock = parentDirEntries[0].location;
    int currBlock = parentStartBlock;
    int blockOffset = 0;
    int blocksWritten = 0;
    int totalBlocks = (vcb->numOfEntries * sizeof(DE) + vcb->blockSize - 1) / vcb->blockSize;
    
    while (currBlock != END_OF_FILE_FLAG && blocksWritten < totalBlocks) {
        LBAwrite(&parentDirEntries[blockOffset], 1, currBlock);
        currBlock = fatTable[currBlock];
        blockOffset += vcb->blockSize / sizeof(DE);
        blocksWritten++;
    }

    free(parentDirEntries);
    return 0;
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
