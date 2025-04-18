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