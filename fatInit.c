/**************************************************************
* Class::  CSC-415-02 Spring 2025
* Name:: 
* Student IDs:: 
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: <name of this file>
*
* Description::
*
**************************************************************/

#include <stdlib.h>
#include <stdio.h>

// initializing the File Allocation Table/FAT inside of an array
void initFAT(int blocks) {
    int *fat = malloc((blocks + 1) * sizeof(int)); // adding 1 block for VCB

    int fatsize = blocks + 1; 

    fat[0] = vcb; //TODO vcb helper function in fsinit.c maybe? 

    // we are setting the free space in the FAT array by
    // initailizing all values to -1 except the first block
    // which is the VCB
    for(int i = 1; i < fatsize; i++) {
        fat[i] = -1; 
    }


    // then we write fat array to disk
    LBAWrite(0, fat, fatsize); 

    // once array is written to disk, we can free from memory
    free(fat);
}

