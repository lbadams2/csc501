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
        bs->bs_status = BSM_UNMAPPED;
        bs->bs_pid = NULL;
        bs->bs_vpno = NULL;
        bs->bs_npages = 0;
        bs->bs_sem = 0;
    }
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
    int i;
    for(i = 0; i < 8; i++) {
        bs_map_t *bs = &bsm_tab[i];
        if(bs->bs_status == BSM_UNMAPPED)
            *avail = i;
            break;
    }
    if(i == 8)
        *avail = -1;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
    bs_map_t *bs = &bsm_tab[i];
    bs->bs_status = BSM_UNMAPPED;
    bs->bs_pid = NULL;
    bs->bs_vpno = NULL;
    bs->bs_npages = 0;
    bs->bs_sem = 0;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
    int i;
    // vpno is upper 20 bits of vaddr
    for(i = 0; i < 8; i++){}

}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
}


