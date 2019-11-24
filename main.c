#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h> 
#include "disk.h"


int blocksize;
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

void read_inode() {
    int in_offset = 1024 + sb.inode_offset * blocksize;
    int data_offset = INODE_START + sb.data_offset * blocksize;
    int j = 0;
    while(in_offset + 100 <= data_offset) {
        inode in;
        printf("printing inode %d\n", j++);
        in.next_inode = readIntAt(buffer + in_offset);
        printf("next inode is %d\n", in.next_inode);
        in.protect = readIntAt(buffer + in_offset + 4);
        printf("protect is %d\n", in.protect);
        in.nlink = readIntAt(buffer + in_offset + 8);
        printf("nlink is %d\n", in.nlink);
        in.size = readIntAt(buffer + in_offset + 12);
        printf("size is %d\n", in.size);
        in.uid = readIntAt(buffer + in_offset + 16);
        printf("uid is %d\n", in.uid);
        in.gid = readIntAt(buffer + in_offset + 20);
        printf("gid is %d\n", in.gid);
        in.ctime = readIntAt(buffer + in_offset + 24);
        printf("ctime is %d\n", in.ctime);
        in.mtime = readIntAt(buffer + in_offset + 28);
        printf("mtime is %d\n", in.mtime);
        in.atime = readIntAt(buffer + in_offset + 32);
        printf("atime is %d\n", in.atime);
        
        int dblock_arr[N_DBLOCKS];
        int i;
        in_offset = in_offset + 36;
        for(i = 0; i < N_DBLOCKS; i++) {
            dblock_arr[i] = readIntAt(buffer + in_offset + i*4);
            printf("dblock %d is %d\n", i, dblock_arr[i]);
        }
        in_offset = in_offset + 40; // + 76
        int iblock_arr[N_IBLOCKS];
        for(i = 0; i < N_IBLOCKS; i++) {
            iblock_arr[i] = readIntAt(buffer + in_offset + i*4);
            printf("iblock %d is %d\n", i, iblock_arr[i]);
        }
        in_offset = in_offset + 16; // + 92
        in.i2block = readIntAt(buffer + in_offset);
        printf("i2block is %d\n", in.i2block);
        in_offset = in_offset + 4;
        in.i3block = readIntAt(buffer + in_offset); // + 96
        printf("i3block is %d\n", in.i3block);
        in_offset += 4; // + 100
        printf("\n\n\n");
    }
}


int main(int argc, char **argv) {
    char *file_name = argv[1];
    read_disk(file_name);
    set_sb();
    read_inode();
    return 0;
}