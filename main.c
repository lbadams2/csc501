#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h> 
#include "disk.h"

int blocksize_int;

void read_disk(char *file_name) {
    FILE *fp;
    size_t bytes;
    fp = fopen(file_name,"r");
    int fd = fileno(fp);
    struct stat finfo;
    fstat(fd, &finfo);
    buffer = malloc(finfo.st_size);
    bytes = fread(buffer,finfo.st_size,1,fp);
}

void set_sb() {
    sb = (superblock*) &(buffer[BBLOCK_SIZE]);
    unsigned long tmp = (unsigned long)sb;
    assert(tmp % 4 == 0);
    assert(sizeof(superblock) == 24);
    assert(sizeof(inode) == 100);
    sb->blocksize = buffer[BBLOCK_SIZE];
    blocksize_int = sb->blocksize / 4;
    printf("block size is %d\n", sb->blocksize);
    sb->inode_offset = buffer[BBLOCK_SIZE + 1];
    printf("inode offset is %d\n", sb->inode_offset);
    sb->data_offset = buffer[BBLOCK_SIZE + 2];
    printf("data offset is %d\n", sb->data_offset);
    sb->swap_offset = buffer[BBLOCK_SIZE + 3];
    printf("swap offset is %d\n", sb->swap_offset);
    sb->free_inode = buffer[BBLOCK_SIZE + 4];
    printf("free inode is %d\n", sb->free_inode);
    sb->free_block = buffer[BBLOCK_SIZE + 5];
    printf("free block is %d\n", sb->free_block);
}

void read_inodes() {
    int in_offset = INODE_START + sb->inode_offset * blocksize_int;
    inode *in =(inode *) &(buffer[in_offset]);
    
    int i, j;
    

    int data_offset = INODE_START + sb->data_offset * blocksize_int;
    for(i = 0; i < data_offset; i+=25) {
        in = in + i;
        printf("printing inode %d\n", i/25);
        printf("next inode is %d\n", in->next_inode);
        for(j = 0; j < N_DBLOCKS; j++) {
            printf("pointer to dblock %d is %d\n", j, in->dblocks[j]);
        }
        for(j = 0; j < N_IBLOCKS; j++) {
            printf("pointer to iblock %d is %d\n", j, in->iblocks[j]);
        }
        printf("\n\n\n");
    }
}


int main(int argc, char **argv) {
    char *file_name = argv[1];
    read_disk(file_name);
    set_sb();
    read_inodes();
    return 0;
}