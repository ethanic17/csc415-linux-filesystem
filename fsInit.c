/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
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

#include "fsLow.h"
#include "mfs.h"

#define TEAM_42_FS_SIGNATURE 42
#define FREE_BLOCK_FLAG -1
#define END_OF_FILE_FLAG -2

typedef struct VolumeControlBlock
{
    int signature; // unique identifier to verify the file system type
    int volumeSize; // size of the volume in bytes
    int blockSize; // size of each block in bytes
    int numOfBlocks; // total number of blocks in the volume
    int fsBlockNum; // block number where the filesystem metadata is
    int rootDirBlockNum; // block number where the root directory is
    int fatStartBlock; // block number where the free space management (FAT) starts
    int fatBlockNum; //number of blocks FAT occupies
    int freeBlockNum; // block number where free space starts
} VCB;

VCB *vcb;
int *fatTable;

int findNextFreeBlock(int startBlockNum)
{
    for (int i = startBlockNum; i < vcb->numOfBlocks; i++)
    {
        if (fatTable[i] == FREE_BLOCK_FLAG)
        {
            return i;
        }
    }

    return -1;
}

int allocateBlocks(int numOfBlocks) {
	int startBlockNum = vcb->freeBlockNum;

	// First check if we have enough free blocks available
	int blockAllocations[numOfBlocks];
	for (int i = 0; i < numOfBlocks; i++)
	{
	    int nextFreeBlock = findNextFreeBlock(i);
	    if (nextFreeBlock == -1)
	    {
	        printf("Failed to allocate blocks.\n");
	        return -1;
	    }
	    blockAllocations[i] = nextFreeBlock;
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

	// Keep track of the new free block number
	vcb->freeBlockNum = findNextFreeBlock(blockAllocations[numOfBlocks - 1]);

	return startBlockNum;
}

int initFreeSpace(int numberOfBlocks, int blockSize)
    {
    int blocksRequiredForFATTable = (numberOfBlocks * sizeof(int) +
                                     (blockSize - 1)) / blockSize;
    fatTable = malloc(numberOfBlocks * sizeof(int));
    if (fatTable == NULL)
	{
	    printf("Error allocating memory for free space.\n");
	}

    fatTable[0] = END_OF_FILE_FLAG;
	for (int i = 1; i < numberOfBlocks; ++i)
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

    vcb->fatStartBlock = 1;
	vcb->freeBlockNum = blocksRequiredForFATTable + 1;

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
	}

	LBAread(vcb, 1, 0);
	if (vcb->signature != TEAM_42_FS_SIGNATURE)
	{
	    vcb->signature = TEAM_42_FS_SIGNATURE;
	    vcb->volumeSize = numberOfBlocks * blockSize;
	    vcb->blockSize = blockSize;
	    vcb->numOfBlocks = numberOfBlocks;

        // Initialize free space
	    int fatBlockNum = initFreeSpace(numberOfBlocks, blockSize);
	    vcb->fatBlockNum = fatBlockNum;

	    LBAwrite(vcb, 1, 0);
	}

	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	free(vcb);
	free(fatTable);
	}
