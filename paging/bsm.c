/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
    bsm_tab[8]; // should be in kernel
    bs_map_t *bs = &bsm_tab[0];
    int i;
    for(i = 0; i < 8; i++) {
        bs->bs_status[NPROC];
        int j;
        for(j = 0; j < NPROC; j++)
            bs->bs_status[j] = BSM_UNMAPPED;
        bs->bs_pid[NPROC];
        for(j = 0; j < NPROC; j++)
            bs->bs_pid[j] = 0;
        bs->bs_vpno[NPROC];
        for(j = 0; j < NPROC; j++)
            bs->bs_vpno[j] = NULL;
        for(j = 0; j < NPROC; j++)
            bs->bs_npages[j] = 0;
        bs->bs_sem = 0;

        int bs_addr = (2048 + i*256)*NBPG;
        bs->free_list = (struct mblock *)bs_addr;
        bs->free_list->mnext = NULL;
        bs->free_list->mlen = 256;
    }
    return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    int i;
    int pid = getpid();
    for(i = 0; i < 8; i++) {
        bs_map_t *bs = &bsm_tab[i];
        if(bs->bs_status[pid] == BSM_UNMAPPED) {
            *avail = i;
            break;
        }
    }
    if(i == 8)
        *avail = -1;
    return OK;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    bs_map_t *bs = &bsm_tab[i];
    int pid = getpid();
    bs->bs_status[pid] = BSM_UNMAPPED;
    bs->bs_pid[pid] = 0;
    bs->bs_vpno[pid] = NULL;
    bs->bs_npages[pid] = 0;
    bs->bs_sem = 0;
    return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    int i;
    // vpno is upper 20 bits of vaddr
    int vpno = vaddr >> 12;
    bs_map_t *bs;
    for(i = 0; i < 8; i++){
        bs = &bsm_tab[i];
        if(bs->bs_pid[pid] == 1) {
            int start_vpno = bs->bs_vpno[pid];
            int npages = bs->bs_npages[pid]; // not sure if this should be per process
            int end_vpno = start_vpno + npages; // unsure of this
            if(vpno >= start_vpno && vpno <= end_vpno) {
                *store = i;
                *pageth = vpno - start_vpno;
                break;
            }
        }
    }
    if(i == 8) {
        *store = -1;
        *pageth = -1;
        return(SYSERR);
    }
    return OK;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
// no store can be mapped to more than one range of virtual memory at a time for a process
// can be mapped to more than one range if ranges are for different processes
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
    bs_map_t *bs = &bsm_tab[source];
    bs->bs_pid[pid] = 1;
    bs->bs_status[pid] = BSM_MAPPED;
    bs->bs_vpno[pid] = vpno;
    bs->bs_npages[pid] = npages;
    return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
// how to handle vpno removed from middle of store? what is flag?
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
    int i;
    bs_map_t *bs;
    for(i = 0; i < 8; i++) {
        bs = &bsm_tab[i];
        if(bs->bs_pid[pid] == 1) {
            bs->bs_status[pid] = BSM_UNMAPPED;
            bs->bs_npages[pid] = 0;
            bs->bs_pid[pid] = 0;
        }
    }
    return OK;
}


