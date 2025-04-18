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