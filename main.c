#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h> 
#include "disk.h"


int blocksize;
int num_inodes;
int total_inodes;
int disk_size;
int ent_per_blk;
int num_data_blocks;
superblock sb;

void read_disk(char *file_name) {
    FILE *fp;
    size_t bytes;
    fp = fopen(file_name,"r");
    int fd = fileno(fp);
    struct stat finfo;
    fstat(fd, &finfo);
    buffer = malloc(finfo.st_size);
    defrag_disk = malloc(finfo.st_size);
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
    total_inodes = (data_offset - in_offset) / sizeof(inode);
    num_inodes = 0;
    inodes = malloc(total_inodes * sizeof(inode));
    printf("total inodes is %d\n", total_inodes);
    int j;
    for(j = 0; j < total_inodes; j++) {
        inodes[j].next_inode = readIntAt(buffer + in_offset);
        inodes[j].protect = readIntAt(buffer + in_offset + 4);
        inodes[j].nlink = readIntAt(buffer + in_offset + 8);
        if(inodes[j].nlink > 0)
            num_inodes++;
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

void init_indirect_map() {
    int i;
    int end = num_data_blocks * ent_per_blk;
    for(i = 0; i < end; i++) {
        indirect_to_direct[i] = -1;
    }
}

// block pointers are relative to start of data block region, so they start at 0
void update_indirect_map(int iblock, int ptr) {
    int start = iblock * ent_per_blk;
    int end = start + ent_per_blk;
    int i;
    for(i = start; i < end; i++) {
        if(indirect_to_direct[i] == -1) {
            indirect_to_direct[i] = ptr;
        }
    }
}

void read_iblock(tmp_node *tnd, int data_offset, int iblock, int lvl) {
    if(lvl == 0) {
        *(tnd->dblocks) = iblock;
        tnd->dblocks++;
        return;
    }
    int blk_ptr, j;
    int ent_per_blk = sb.blocksize / 4;
    j = 0;
    // need to remember what iblock was pointing to
    *(tnd->iblocks) = iblock;
    tnd->iblocks++;
    tnd->num_iblocks++;
    blk_ptr = readIntAt(buffer + data_offset + iblock);
    while(blk_ptr != -1 && j < ent_per_blk) {
        direct_to_indirect[blk_ptr] = iblock;
        update_indirect_map(iblock, blk_ptr);
        read_iblock(tnd, data_offset, blk_ptr, lvl - 1);
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

void store_inode_data(tmp_node *tnd, int num_blks, int data_offset) {
    int i, j, iblock, dblock, data;
    //tmp_node *tnd = malloc(2*num_blks + sb.blocksize * 2 * num_blks);
    tmp_node *tnd;
    //size_t tnd_size = sb.blocksize * num_blks + sb.blocksize * num_blks + num_blks * sizeof(int) + num_blks * sizeof(int);
    for(i = 0; i < num_blks; i++) {
        iblock = tnd->iblocks[i];
        dblock = tnd->dblocks[i];
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
tmp_node **traverse_inodes() {
    int data_offset = INODE_START + sb.data_offset * blocksize;
    int swp_offset = INODE_START + sb.swap_offset * blocksize;
    num_data_blocks = (swp_offset - data_offset) / blocksize;
    ent_per_blk = sb.blocksize / 4; // ints per block
    direct_to_indirect = malloc(num_data_blocks * sizeof(int));
    indirect_to_direct = malloc(num_data_blocks * sizeof(int) * ent_per_blk);
    int i, j, blk_ptr, next_free_block = 0;
    int *inode_data_blocks;
    int *inode_indirect_blocks;
    inode *in;
    tmp_node *tnd;
    tmp_node *tmp_nodes[num_inodes];
    for(i = 0; i < total_inodes; i++) {
        in = &inodes[i];
        //in->size
        if(in->nlink > 0) { // inode being used
            inode_data_blocks = malloc(num_data_blocks * sizeof(int)); // max blocks file can use
            inode_indirect_blocks = malloc(num_data_blocks * sizeof(int)); // max blocks file can use
            init_blks(inode_indirect_blocks, inode_data_blocks, num_data_blocks);
            tnd = malloc(sizeof(tmp_node));
            tnd->inode_num = i;
            tnd->iblocks = inode_indirect_blocks;
            tnd->dblocks = inode_data_blocks;
            tnd->dblk_data = malloc(sb.blocksize * num_data_blocks);
            tnd->iblk_data = malloc(sb.blocksize * num_data_blocks);
            tnd->num_dblocks = 0;
            tnd->num_iblocks = 0;
            // go through indirect blocks first
            if(in->i3block != -1)
                read_iblock(tnd, data_offset, in->i3block, 3);
            if(in->i2block != -1)
                read_iblock(tnd, data_offset, in->i2block, 2);
            for(j = 0; j < N_IBLOCKS; j++)
                if(in->iblocks[j] != -1)
                    read_iblock(tnd, data_offset, in->iblocks[j], 1);
            for(j = 0; j < N_DBLOCKS; j++)
                if(in->dblocks[j] != -1) {
                    *(tnd->dblocks) = in->dblocks[j];
                    tnd->dblocks++;
                }
        store_inode_data(tnd, num_data_blocks, data_offset);
        tmp_nodes[i] = tnd;
        // get all tnd's then write them all to new disk contiguously
        }
    }
    return tmp_nodes;
}

// keep same inode numbers, so inode region still has gaps
// unused inodes have -1 for pointers and 0 for other fields, unused bytes at end of region are 0
// free data block has 0s after first byte containing pointer, last used block for file has 0s for bytes beyond file size
void write_new_disk(tmp_node **tmp_nodes) {
    int i, j, k;
    //copy boot block
    for(i = 0; i < 512; i++)
        defrag_disk[i] = buffer[i];

    int data_offset = INODE_START + sb.data_offset * blocksize;
    int next_free = sb.data_offset;
    tmp_node *tnd;
    inode *def_in;
    inode *defrag_inodes[total_inodes];
    int in_data_blk_start, def_blk, data, in_data_blk_end;
    for(i = 0; i < num_inodes; i++) {
        tnd = tmp_nodes[i];
        def_in = malloc(sizeof(inode));
        in_data_blk_start = next_free + tnd->num_iblocks;
        in_data_blk_end = in_data_blk_start + tnd->num_dblocks;
        for(j = 0; j < tnd->num_dblocks; j++) {
            def_blk = tnd->dblocks[j]; 
            for(k = 0; k < ent_per_blk; k++) {
                data = readIntAt(buffer + data_offset + def_blk + k*4);
                writeIntAt(defrag_disk + data_offset + in_data_blk_start + k*4, data);
            }
            in_data_blk_start++;
        }
        // indirect blocks come before data blocks
        for(j = 0; j < tnd->num_iblocks; j++) {

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
                    direct_block = readIntAt(buffer + data_offset + idb*blocksize);
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
        fb = readIntAt(buffer + data_offset + fb * blocksize);
        printf("next free block is %d\n", fb);
    }
    printf("num free blocks is %d\n", num_free_blocks);
}


int main(int argc, char **argv) {
    char *file_name = argv[1];
    read_disk(file_name);
    set_sb();
    read_inodes();
    //print_inodes();
    //defrag();
    //get_data_blocks();
    tmp_node **tmp_node_arr = traverse_inodes();
    return 0;
}