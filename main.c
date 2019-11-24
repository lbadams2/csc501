#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h> 
#include "disk.h"


int blocksize;
int num_inodes;
superblock sb;

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

// puts most signifcant byte (p+3) in bits 24-31, etc
int readIntAt(unsigned char *p) {
    return *(p+3) * 256 * 256 * 256 + *(p+2) * 256 * 256 + *(p+1) * 256 + *p;
}

void set_sb() {    
    unsigned long tmp = (unsigned long)(&sb);
    assert(tmp % 4 == 0);
    assert(sizeof(superblock) == 24);
    assert(sizeof(inode) == 100);
    sb.blocksize = readIntAt(buffer + 512);
    blocksize = sb.blocksize;
    printf("block size is %d\n", sb.blocksize);
    sb.inode_offset = readIntAt(buffer + 512 + 4);
    printf("inode offset is %d\n", sb.inode_offset);
    sb.data_offset = readIntAt(buffer + 512 + 8);
    printf("data offset is %d\n", sb.data_offset);
    sb.swap_offset = readIntAt(buffer + 512 + 12);
    printf("swap offset is %d\n", sb.swap_offset);
    sb.free_inode = readIntAt(buffer + 512 + 16);
    printf("free inode is %d\n", sb.free_inode);
    sb.free_block = readIntAt(buffer + 512 + 20);
    printf("free block is %d\n", sb.free_block);
}

void read_inodes() {
    int in_offset = INODE_START + sb.inode_offset * blocksize;
    int data_offset = INODE_START + sb.data_offset * blocksize;
    num_inodes = (data_offset - in_offset) / sizeof(inode);
    inodes = malloc(num_inodes * sizeof(inode));
    printf("num inodes is %d\n", num_inodes);
    int j;
    for(j = 0; j < num_inodes; j++) {
        inodes[j].next_inode = readIntAt(buffer + in_offset);
        inodes[j].protect = readIntAt(buffer + in_offset + 4);
        inodes[j].nlink = readIntAt(buffer + in_offset + 8);
        inodes[j].size = readIntAt(buffer + in_offset + 12);
        inodes[j].uid = readIntAt(buffer + in_offset + 16);
        inodes[j].gid = readIntAt(buffer + in_offset + 20);
        inodes[j].ctime = readIntAt(buffer + in_offset + 24);
        inodes[j].mtime = readIntAt(buffer + in_offset + 28);
        inodes[j].atime = readIntAt(buffer + in_offset + 32);
        
        int dblock_arr[N_DBLOCKS];
        int i;
        in_offset = in_offset + 36;
        for(i = 0; i < N_DBLOCKS; i++) {
            inodes[j].dblocks[i] = readIntAt(buffer + in_offset + i*4);
        }
        in_offset = in_offset + 40; // + 76
        int iblock_arr[N_IBLOCKS];
        for(i = 0; i < N_IBLOCKS; i++) {
            inodes[j].iblocks[i] = readIntAt(buffer + in_offset + i*4);
        }
        in_offset = in_offset + 16; // + 92
        inodes[j].i2block = readIntAt(buffer + in_offset);
        in_offset = in_offset + 4;
        inodes[j].i3block = readIntAt(buffer + in_offset); // + 96
        in_offset += 4; // + 100
    }
}

void print_inode(int i) {
    printf("next inode is %d\n", inodes[i].next_inode);
    printf("protect is %d\n", inodes[i].protect);
    printf("nlink is %d\n", inodes[i].nlink);
    printf("size is %d\n", inodes[i].size);
    printf("uid is %d\n", inodes[i].uid);
    printf("gid is %d\n", inodes[i].gid);
    printf("ctime is %d\n", inodes[i].ctime);
    printf("mtime is %d\n", inodes[i].mtime);
    printf("atime is %d\n", inodes[i].atime);
    int j;
    for(j = 0; j < N_DBLOCKS; j++) {
        printf("dblock %d is %d\n", j, inodes[i].dblocks[j]);
    }
    for(j = 0; j < N_IBLOCKS; j++) {
        printf("iblock %d is %d\n", j, inodes[i].dblocks[j]);
    }
    printf("i2block is %d\n", inodes[i].i2block);
    printf("i3block is %d\n", inodes[i].i3block);
}

void print_inodes() {
    int i;
    for(i = 0; i < num_inodes; i++) {
        printf("\nprinting inode %d\n", i);
        print_inode(i);
        printf("\n\n");
    }
}


int main(int argc, char **argv) {
    char *file_name = argv[1];
    read_disk(file_name);
    set_sb();
    read_inodes();
    print_inodes();
    return 0;
}