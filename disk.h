#ifndef _DISK_H_
#define _DISK_H_

#define N_DBLOCKS 10 
#define N_IBLOCKS 4 
#define BBLOCK_SIZE 512
#define INODE_START 1024

typedef struct {
    int blocksize; /* size of blocks in bytes */
    int inode_offset; /* offset of inode region in blocks */
    int data_offset; /* data region offset in blocks */
    int swap_offset; /* swap region offset in blocks */
    int free_inode; /* head of free inode list */
    int free_block; /* head of free block list */
} superblock;

typedef struct {  
      int next_inode; /* list for free inodes */  
      int protect;        /*  protection field */ 
      int nlink;  /* Number of links to this file */ 
      int size;  /* Number of bytes in file */   
      int uid;   /* Owner's user ID */  
      int gid;   /* Owner's group ID */  
      int ctime;  /* Time field */  
      int mtime;  /* Time field */  
      int atime;  /* Time field */  
      int dblocks[N_DBLOCKS];   /* Pointers to data blocks */  
      int iblocks[N_IBLOCKS];   /* Pointers to indirect blocks */  
      int i2block;     /* Pointer to doubly indirect block */  
      int i3block;     /* Pointer to triply indirect block */  
} inode;

typedef struct {
    int inode_num;
    int num_iblocks;
    int num_dblocks;
    int *iblocks;
    int *dblocks;
    int *iblk_data;
    int *dblk_data;
} tmp_node;

unsigned char *buffer;
unsigned char *defrag_disk;
int *indirect_to_direct;
int *direct_to_indirect;
inode *inodes;
extern superblock sb;
extern int blocksize;
extern int num_inodes;
extern int disk_size;
extern int total_inodes;
extern int ent_per_blk;
extern int num_data_blocks;
extern size_t file_size;

#endif