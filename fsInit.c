/**************************************************************
* Class::  CSC-415-02 Spring 2025
* Name:: Nabeel Rana, Leigh Ann Apotheker, Ethan Zheng, Bryan Mendez
* Student IDs:: 924432311, 923514173, 922474550, 922744724
* GitHub-Name:: nabware
* Group-Name:: Team 42
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
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

int initRootDirectory()
    {
    int numOfEntries = 50;
    int requiredNumOfBytes = numOfEntries * sizeof(DE);
    int requiredNumOfBlocks = (requiredNumOfBytes + (vcb->blockSize - 1)) /
                              vcb->blockSize;

    // Check if you can fit more entries into the number of blocks we're asking for.
    int extraEntries = ((requiredNumOfBlocks * vcb->blockSize) -
                        requiredNumOfBytes) / sizeof(DE);
    numOfEntries += extraEntries;
    requiredNumOfBytes += extraEntries * sizeof(DE);

    DE *directoryEntries = malloc(requiredNumOfBytes);
    if (directoryEntries == NULL)
    {
        printf("Error allocating memory for directory entries.\n");
		return -1;
    }
    for (int i = 0; i < numOfEntries; i++)
    {
        directoryEntries[i].used = 0;
    }

    // Initialize the two directories "." and ".."
    int startBlock = allocateBlocks(requiredNumOfBlocks);
    strcpy(directoryEntries[0].name, ".");
    strcpy(directoryEntries[1].name, "..");
    time_t timeNow;
    time(&timeNow);
    for (int i = 0; i < 2; i++)
    {
        directoryEntries[i].used = 1;
        directoryEntries[i].location = startBlock;
        directoryEntries[i].size = 0;
        directoryEntries[i].isDirectory = 1;
        directoryEntries[i].createdAt = timeNow;
        directoryEntries[i].modifiedAt = timeNow;
    }

    LBAwrite(directoryEntries, requiredNumOfBlocks, startBlock);

    free(directoryEntries);

    vcb->numOfEntries = numOfEntries;

    return startBlock;
    }
/**
int readDirectory(int blockNum, DE **entries)
{
    if (blockNum < 0 || blockNum >= vcb->numOfBlocks)
    {
        printf("Block number is invalid.\n");
	return -1;
    }
    
    int currentBlock = blockNum;
    int numBlocks = 0;
    //Use FAT to get total size
    while (currentBlock != END_OF_FILE_FLAG)
    {
        numBlocks++;
        currentBlock = fatTable[currentBlock];
    }
    
    // Calculate how many directory entries to be stored
    int bufferSize = numBlocks * vcb->blockSize;
    int maxEntries = bufferSize / sizeof(DE);
    
    *entries = malloc(bufferSize);
    if (*entries == NULL)
    {
        printf("Allocating memory error.\n");
        return -1;
    }
    
    // Read all blocks of the directory into the buffer
    if (LBAread(*entries, numBlocks, blockNum) != numBlocks)
    {
        printf("Reading directory blocks error.\n");
        free(*entries);
        *entries = NULL;
        return -1;
    }
    
    // Count how many entries are used
    int usedEntries = 0;
    for (int i = 0; i < maxEntries; i++)
    {
        if ((*entries)[i].used)
        {
            usedEntries++;
        }
    }
    
    return usedEntries;
}
**/

int initFreeSpace()
    {
    int blocksRequiredForFATTable = (vcb->numOfBlocks * sizeof(int) +
                                     (vcb->blockSize - 1)) / vcb->blockSize;
    fatTable = malloc(vcb->numOfBlocks * sizeof(int));
    if (fatTable == NULL)
	{
	    printf("Error allocating memory for free space.\n");
		return -1;
	}

    fatTable[0] = END_OF_FILE_FLAG;
	for (int i = 1; i < vcb->numOfBlocks; i++)
	{
	    if (i < blocksRequiredForFATTable)
	    {
	        fatTable[i] = i + 1;
	    }
	    else if (i == blocksRequiredForFATTable)
	    {
	        fatTable[i] = END_OF_FILE_FLAG;
	    }
	    else
	    {
	        fatTable[i] = FREE_BLOCK_FLAG;
	    }
	}

	LBAwrite(fatTable, blocksRequiredForFATTable, 1);

    vcb->fatBlockNum = 1;
	vcb->freeBlockNum = 1 + blocksRequiredForFATTable;

	return blocksRequiredForFATTable;
    }

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	vcb = malloc(blockSize);
	if (vcb == NULL)
	{
	    printf("Error allocating memory for VCB.\n");
		return -1;
	}

	LBAread(vcb, 1, 0);
	if (vcb->signature != TEAM_42_FS_SIGNATURE)
	{
	    vcb->signature = TEAM_42_FS_SIGNATURE;
	    vcb->volumeSize = numberOfBlocks * blockSize;
	    vcb->blockSize = blockSize;
	    vcb->numOfBlocks = numberOfBlocks;
		vcb->fsBlockNum = 0;

        // Initialize free space
	    int fatNumOfBlocks = initFreeSpace();
		if (fatNumOfBlocks == -1)
		{
			return -1;
		}
	    vcb->fatNumOfBlocks = fatNumOfBlocks;

	    // Initialize root directory
	    int rootDirBlockNum = initRootDirectory();
		if (rootDirBlockNum == -1)
		{
			return -1;
		}
	    vcb->rootDirBlockNum = rootDirBlockNum;

	    LBAwrite(vcb, 1, 0);
	}
    else
	{
		fatTable = malloc(vcb->fatNumOfBlocks * vcb->blockSize);
		if (fatTable == NULL)
		{
			printf("Error allocating memory for free space.\n");
			return -1;
		}
		LBAread(fatTable, vcb->fatNumOfBlocks, vcb->fatBlockNum);
	}

	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	free(vcb);
	free(fatTable);
	}
