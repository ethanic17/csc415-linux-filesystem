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
} VCB;

VCB *vcb;

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

	    LBAwrite(vcb, 1, 0);
	}

	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	free(vcb);
	}
