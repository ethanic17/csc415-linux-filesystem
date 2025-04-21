/**************************************************************
* Class::  CSC-415-02 Spring 2025
* Name:: Nabeel Rana, Leigh Ann Apotheker, Ethan Zheng, Bryan Mendez
* Student IDs:: 924432311, 923514173, 922474550, 922744724
* GitHub-Name:: nabware
* Group-Name:: Team 42
* Project:: Basic File System
*
* File:: mfs.h
*
* Description:: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/


#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"

#include <dirent.h>
#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

#define TEAM_42_FS_SIGNATURE 42
#define FREE_BLOCK_FLAG -1
#define END_OF_FILE_FLAG -2
#define PATHMAX_LEN 4096

typedef struct VolumeControlBlock
{
    int signature; // unique identifier to verify the file system type
    int volumeSize; // size of the volume in bytes
    int blockSize; // size of each block in bytes
    int numOfBlocks; // total number of blocks in the volume
    int fsBlockNum; // block number where the filesystem metadata is
    int rootDirBlockNum; // block number where the root directory is
    int fatBlockNum; // block number where the free space management (FAT) starts
    int fatNumOfBlocks; // number of blocks FAT occupies
    int freeBlockNum; // block number where free space starts
	int numOfEntries; // number of entries per directory
} VCB;

typedef struct DirectoryEntry
{
    char name[100]; // name of the file
    int size; // size of the file in bytes
    int location; // block number where the file is
    int isDirectory; // flag to indicate if this is a directory or not
    int used; // flag to indicate if the directory entry is used
    time_t createdAt; // timestamp when file was created
    time_t modifiedAt; // timestamp when file was last modified
} DE;

extern VCB *vcb;
extern int *fatTable;
extern char cwd[PATHMAX_LEN];

// This structure is returned by fs_readdir to provide the caller with information
// about each file as it iterates through a directory
struct fs_diriteminfo
	{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
	};

// This is a private structure used only by fs_opendir, fs_readdir, and fs_closedir
// Think of this like a file descriptor but for a directory - one can only read
// from a directory.  This structure helps you (the file system) keep track of
// which directory entry you are currently processing so that everytime the caller
// calls the function readdir, you give the next entry in the directory
typedef struct
	{
	/*****TO DO:  Fill in this structure with what your open/read directory needs  *****/
	unsigned short  d_reclen;		/* length of this record */
	unsigned short	dirEntryPosition;	/* which directory entry position, like file pos */
	DE *	directory;			/* Pointer to the loaded directory you want to iterate */
	struct fs_diriteminfo * di;		/* Pointer to the structure you return from read */
	} fdDir;

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
int fs_isFile(const char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(const char * pathname);		//return 1 if directory, 0 otherwise
int fs_create(char* filename); // creates a file
int fs_delete(char* filename);	//removes a file
int fs_move(char* src, char* dest);	// moves a file

// Helper functions
int findNextFreeBlock(int startBlockNum);
int allocateBlocks(int startBlock, int numOfBlocks); // allocates blocks and returns start block
void readDirEntries(DE* dirEntries, int startBlock);
void writeDirEntries(DE* dirEntries, int startBlock);
DE* getDirEntriesFromPath(const char *pathname);
DE* getDirEntryFromPath(const char *pathname); // returns directory entry
void getAbsolutePath(char* absolutePath, const char *pathname);
void getParentPath(char* parentPath, char *pathname);
void getLastElementName(char * elementName, char * path);

// This is the strucutre that is filled in from a call to fs_stat
struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	// blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	
	/* add additional attributes here for your file system */
	};

int fs_stat(const char *path, struct fs_stat *buf);

#endif

