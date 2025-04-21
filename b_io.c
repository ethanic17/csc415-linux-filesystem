/**************************************************************
* Class::  CSC-415-02 Spring 2025
* Name:: Nabeel Rana, Leigh Ann Apotheker, Ethan Zheng, Bryan Mendez
* Student IDs:: 924432311, 923514173, 922474550, 922744724
* GitHub-Name:: nabware
* Group-Name:: Team 42
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "mfs.h"
#include "fsLow.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
	{
	/** TODO add al the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	int currBlock; // holds the current block number on disk
	int flags;
	int remainingBytesInFile;
	DE * parentDirEntries;
	int parentDirEntriesLocation;
	DE * fileDirEntry;
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
										// check for error - all used FCB's
	// No available FCBs
	if (returnFd == -1)
	{
		return -1;
	}
	
	int isDir = fs_isDir(filename);
	int isFile = fs_isFile(filename);
	// filename is a directory or file does not exist and create flag is missing
	if (isDir == 1 || (isFile == 0 && !(flags & O_CREAT)))
	{
		return -1;
	}

	// Create file if it doesn't exist
	if (isFile == 0 && flags & O_CREAT)
	{
		int result = fs_create(filename);
		if (result == -1)
		{
			return -1;
		}
	}

	// Get the parent dir entries
	char absolutePath[PATHMAX_LEN];
	getAbsolutePath(absolutePath, filename);
	char parentPath[PATHMAX_LEN];
	getParentPath(parentPath, absolutePath);
	DE* parentDirEntry = getDirEntryFromPath(parentPath);
	if (parentDirEntry == NULL)
	{
		return -1;
	}
	DE* parentDirEntries = getDirEntriesFromPath(parentPath);
	if (parentDirEntries == NULL)
	{
		free(parentDirEntry);
		return -1;
	}
	DE* fileDirEntry = NULL;
	char fileName[PATHMAX_LEN];
	getLastElementName(fileName, absolutePath);
	for (int i = 0; i < vcb->numOfEntries; i++)
	{
		if (parentDirEntries[i].used == 1 && strcmp(parentDirEntries[i].name, fileName) == 0)
		{
			fileDirEntry = &parentDirEntries[i];
			break;
		}
	}
	if (fileDirEntry == NULL)
	{
		free(parentDirEntry);
		free(parentDirEntries);
		return -1;
	}
	// Initialize the FCB
	fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
	if (fcbArray[returnFd].buf == NULL)
	{
		free(parentDirEntry);
		free(parentDirEntries);
		return -1;
	}
	fcbArray[returnFd].index = 0;
	fcbArray[returnFd].buflen = 0;
	fcbArray[returnFd].currBlock = fileDirEntry->location;
	fcbArray[returnFd].flags = flags;
	fcbArray[returnFd].parentDirEntries = parentDirEntries;
	fcbArray[returnFd].parentDirEntriesLocation = parentDirEntry->location;
	fcbArray[returnFd].fileDirEntry = fileDirEntry;
	if (flags & O_TRUNC)
	{
		fileDirEntry->size = 0;
		fcbArray[returnFd].remainingBytesInFile = fileDirEntry->size;
	}
	else
	{
		fcbArray[returnFd].remainingBytesInFile = fileDirEntry->size;
	}

	free(parentDirEntry);

	return (returnFd);						// all set
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	if (fcbArray[fd].buf == NULL)		//File not open for this descriptor
		{
		return -1;
		}
	int bytesCopied = 0;
	int srcBufferIndex = 0;
	int remainingBytesToCopy = count;
	int availableBytesInBuffer = B_CHUNK_SIZE - fcbArray[fd].buflen;
	while (remainingBytesToCopy > 0)
	{
		// If buffer is full, write it to disk
		if (fcbArray[fd].buflen == B_CHUNK_SIZE)
		{
			if (fatTable[fcbArray[fd].currBlock] == END_OF_FILE_FLAG)
			{
				allocateBlocks(fcbArray[fd].currBlock, 2);
			}
			LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].currBlock);
			fcbArray[fd].fileDirEntry->size += fcbArray[fd].buflen;
			fcbArray[fd].currBlock = fatTable[fcbArray[fd].currBlock];
			fcbArray[fd].buflen = 0;
			fcbArray[fd].index = 0;
			availableBytesInBuffer = B_CHUNK_SIZE;
		}

		// Otherwise, write what you can into dest buffer
		int bytesToCopy = remainingBytesToCopy < availableBytesInBuffer ? remainingBytesToCopy : availableBytesInBuffer;
		memcpy(&fcbArray[fd].buf[fcbArray[fd].index], &buffer[srcBufferIndex], bytesToCopy);
		bytesCopied += bytesToCopy;
		remainingBytesToCopy -= bytesToCopy;
		srcBufferIndex += bytesToCopy;
		fcbArray[fd].index += bytesToCopy;
		fcbArray[fd].buflen += bytesToCopy;
	}

	return (bytesCopied); //Change this
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	int bytesCopied = 0;
	int destBufferIndex = 0;
	int remainingBytesToCopy = count;
	int remainingBytesInBuffer = fcbArray[fd].buflen - fcbArray[fd].index;

	// If we've previously loaded the buffer with a block, give as much data
    // from here before having to read additional blocks.
	if (remainingBytesToCopy > 0 && remainingBytesInBuffer > 0)
	{
		int bytesToCopy = remainingBytesToCopy < remainingBytesInBuffer ? remainingBytesToCopy : remainingBytesInBuffer;
		memcpy(&buffer[destBufferIndex], &fcbArray[fd].buf[fcbArray[fd].index], bytesToCopy);
		bytesCopied += bytesToCopy;
		remainingBytesToCopy -= bytesToCopy;
		remainingBytesInBuffer -= bytesToCopy;
		destBufferIndex += bytesToCopy;
		fcbArray[fd].index += bytesToCopy;
    }

	while (remainingBytesToCopy > B_CHUNK_SIZE &&
		   fcbArray[fd].remainingBytesInFile > B_CHUNK_SIZE)
	{
		int readBlocks = LBAread(&buffer[destBufferIndex], 1, fcbArray[fd].currBlock);
		bytesCopied += readBlocks * B_CHUNK_SIZE;
		remainingBytesToCopy -= readBlocks * B_CHUNK_SIZE;
		destBufferIndex += readBlocks * B_CHUNK_SIZE;
		fcbArray[fd].remainingBytesInFile -= readBlocks * B_CHUNK_SIZE;
		fcbArray[fd].currBlock = fatTable[fcbArray[fd].currBlock];
	}

	if (remainingBytesToCopy > 0 && fcbArray[fd].remainingBytesInFile > 0)
	{
		LBAread(fcbArray[fd].buf, 1, fcbArray[fd].currBlock);
		fcbArray[fd].currBlock = fatTable[fcbArray[fd].currBlock];
		fcbArray[fd].buflen = B_CHUNK_SIZE < fcbArray[fd].remainingBytesInFile ? B_CHUNK_SIZE : fcbArray[fd].remainingBytesInFile;
		remainingBytesInBuffer = fcbArray[fd].remainingBytesInFile;
		fcbArray[fd].remainingBytesInFile -= fcbArray[fd].buflen;
		fcbArray[fd].index = 0;

		int bytesToCopy = remainingBytesToCopy < remainingBytesInBuffer ?
						  remainingBytesToCopy : remainingBytesInBuffer;
		memcpy(&buffer[destBufferIndex], &fcbArray[fd].buf[fcbArray[fd].index], bytesToCopy);
		bytesCopied += bytesToCopy;
		remainingBytesToCopy -= bytesToCopy;
		remainingBytesInBuffer -= bytesToCopy;
		destBufferIndex += bytesToCopy;
		fcbArray[fd].index += bytesToCopy;
	}

	return (bytesCopied);	//Change this
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{
		if (fcbArray[fd].buf == NULL)
		{
			return 0;
		}

		// If this was a write request, and theres anything left in the buffer,
		// write it to disk
		if ((fcbArray[fd].flags & O_WRONLY || fcbArray[fd].flags & O_RDWR) &&
			fcbArray[fd].index > 0)
		{
			if (fatTable[fcbArray[fd].currBlock] == END_OF_FILE_FLAG)
			{
				allocateBlocks(fcbArray[fd].currBlock, 2);
			}
			LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].currBlock);
			fcbArray[fd].currBlock = fatTable[fcbArray[fd].currBlock];
			fcbArray[fd].fileDirEntry->size += fcbArray[fd].buflen;
			fcbArray[fd].buflen = 0;
			writeDirEntries(fcbArray[fd].parentDirEntries, fcbArray[fd].parentDirEntriesLocation);
		}

		free(fcbArray[fd].parentDirEntries);
		free(fcbArray[fd].buf);
		fcbArray[fd].buf = NULL;

		return 0;
	}
