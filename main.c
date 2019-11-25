#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h> 
#include "disk.h"


int blocksize;
int num_inodes;
int disk_size;
superblock sb;

void read_disk(char *file_name) {
    FILE *fp;
    size_t bytes;
    fp = fopen(file_name,"r");
    int fd = fileno(fp);
    struct stat finfo;
    fstat(fd, &finfo);
    buffer = malloc(finfo.st_size);
    disk_size = finfo.st_size;
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
    int total_file_bytes = 0;
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
        
        total_file_bytes += inodes[j].size;
        int i;
        in_offset = in_offset + 36;
        for(i = 0; i < N_DBLOCKS; i++) {
            inodes[j].dblocks[i] = readIntAt(buffer + in_offset + i*4);
        }
        in_offset = in_offset + 40; // + 76
        for(i = 0; i < N_IBLOCKS; i++) {
            inodes[j].iblocks[i] = readIntAt(buffer + in_offset + i*4);
        }
        in_offset = in_offset + 16; // + 92
        inodes[j].i2block = readIntAt(buffer + in_offset);
        in_offset = in_offset + 4;
        inodes[j].i3block = readIntAt(buffer + in_offset); // + 96
        in_offset += 4; // + 100
    }
    printf("total size of files is %d bytes\n", total_file_bytes);
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
        printf("iblock %d is %d\n", j, inodes[i].iblocks[j]);
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

void read_iblock(int *iblocks, int *dblocks, int data_offset, int iblock, int lvl) {
    if(lvl == 0) {
        *dblocks = iblock;
        dblocks++;
        return;
    }
    int blk_ptr, j;
    int ent_per_blk = sb.blocksize / 4;
    j = 0;
    *iblocks = iblock;
    iblocks++;
    blk_ptr = readIntAt(buffer + data_offset + iblock);
    while(blk_ptr != -1 && j < ent_per_blk) {
        read_iblock(iblocks, dblocks, data_offset, blk_ptr, lvl - 1);
        j++;
        blk_ptr = readIntAt(buffer + data_offset + iblock + j*4);
    }
}

void init_blks(int *iblocks, int *dblocks, int size) {
    int i;
    for(i = 0; i < size; i++) {
        iblocks[i] = -1;
        dblocks[i] = -1;
    }
}

tmp_node *move_blocks(int *iblocks, int *dblocks, int num_blks, int ent_per_blk, int data_offset) {
    int i, j, iblock, dblock, data;
    //tmp_node *tnd = malloc(2*num_blks + sb.blocksize * 2 * num_blks);
    tmp_node *tnd;
    tnd->dblk_data = malloc(sb.blocksize * num_blks);
    tnd->iblk_data = malloc(sb.blocksize * num_blks);
    tnd->dblocks = malloc(num_blks * sizeof(int));
    tnd->iblocks = malloc(num_blks * sizeof(int));
    for(i = 0; i < num_blks; i++) {
        iblock = iblocks[i];
        dblock = dblocks[i];
        if(iblock != -1)
            for(j = 0; j < ent_per_blk; j++) {
                data = readIntAt(buffer + data_offset + iblock + j*4);
                *(tnd->iblk_data) = data;
                tnd->iblk_data++;
            }
        if(dblock != -1)
            for(j = 0; j < ent_per_blk; j++) {
                data = readIntAt(buffer + data_offset + dblock + j*4);
                *(tnd->dblk_data) = data;
                tnd->dblk_data++;
            }
        if(dblock == -1 && iblock == -1)
            break;
    }
    return tnd;
}


// place indirect blocks before data blocks for each file, not sure if they should be contiguous
void load_file_data() {
    int data_offset = INODE_START + sb.data_offset * blocksize;
    int swp_offset = INODE_START + sb.swap_offset * blocksize;
    int num_data_blocks = (swp_offset - data_offset) / blocksize;
    int i, j, blk_ptr, next_free_block = 0;
    int ent_per_blk = sb.blocksize / 4; // ints per block
    int *inode_data_blocks;
    int *inode_indirect_blocks;
    inode *in;
    tmp_node *tnd;
    for(i = 0; i < num_inodes; i++) {
        in = &inodes[i];
        //in->size
        if(in->nlink > 0) { // inode being used
            inode_data_blocks = malloc(num_data_blocks * sizeof(int)); // max blocks file can use
            inode_indirect_blocks = malloc(num_data_blocks * sizeof(int)); // max blocks file can use
            init_blks(inode_indirect_blocks, inode_data_blocks, num_data_blocks);
            // go through indirect blocks first
            if(in->i3block != -1)
                read_iblock(inode_indirect_blocks, inode_data_blocks, data_offset, in->i3block, 3);
            if(in->i2block != -1)
                read_iblock(inode_indirect_blocks, inode_data_blocks, data_offset, in->i2block, 2);
            for(j = 0; j < N_IBLOCKS; j++)
                if(in->iblocks[j] != -1)
                    read_iblock(inode_indirect_blocks, inode_data_blocks, data_offset, in->iblocks[j], 1);
            for(j = 0; j < N_DBLOCKS; j++)
                if(in->dblocks[j] != -1) {
                    *inode_data_blocks = in->dblocks[j];
                    inode_data_blocks++;
                }
        tnd = (inode_indirect_blocks, inode_data_blocks, num_data_blocks, ent_per_blk, data_offset);
        // get all tnd's then write them all to new disk contiguously
        }
    }
}

// indirect block holds pointers to data blocks, if blocksize is 512 can have 128 pointers
void get_data_blocks() {
    int i, j;
    inode *in;
    int found_one = 0, idb, direct_block;
    printf("inode start is %d, sb data offset is %d, blocksize is %d\n", INODE_START, sb.data_offset, blocksize);
    int data_offset = INODE_START + sb.data_offset * blocksize;
    for(i = 0; i < num_inodes; i++) {
        in = &inodes[i];        
        if(in->nlink > 0) { // inode being used
            found_one = 1;
            printf("processing inode %d\n", i);
            for(j = 0; j < N_IBLOCKS; j++) {
                idb = in->iblocks[j];
                if(idb != -1) {
                    printf("processing inode %d indirect block %d\n", i, j);
                    int bs = readIntAt(buffer + 512);
                    int fs = readIntAt(buffer + INODE_START + 12);
                    printf("block size is %d, file size is %d\n", bs, fs);
                    direct_block = readIntAt(buffer + data_offset + idb);
                    printf("direct block in first entry of indirect block %d is %d for inode %d\n", idb, direct_block, i);
                }
            }
        }
        //if(found_one)
        //    break;
    }
}

// block pointers are indexes relative to start of data block region
// first 4 bytes of free block are index of next free block
void defrag() {
    int data_offset = INODE_START + sb.data_offset * blocksize;
    int swp_offset = INODE_START + sb.swap_offset * blocksize;
    int num_data_blocks = (swp_offset - data_offset) / blocksize;
    int num_swap_blocks = (disk_size - swp_offset) / blocksize;
    printf("\n\nnum data blocks is %d, %d bytes\n", num_data_blocks, num_data_blocks*blocksize);
    printf("data block starts at %d\n", data_offset / blocksize);
    printf("num swap blocks is %d\n", num_swap_blocks);
    int fb = sb.free_block;
    printf("head free block is %d\n", fb);
    int num_free_blocks = 0;
    while(fb != -1) {
        num_free_blocks++;
        fb = readIntAt(buffer + data_offset + fb);
        printf("next free block is %d\n", fb);
    }
    printf("num free blocks is %d\n", num_free_blocks);
}


int main(int argc, char **argv) {
    char *file_name = argv[1];
    read_disk(file_name);
    set_sb();
    read_inodes();
    print_inodes();
    //defrag();
    get_data_blocks();
    return 0;
}