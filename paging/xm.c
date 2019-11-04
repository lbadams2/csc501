/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


unsigned long getvmem(struct vmblock *, int, int);
/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
// different calls to same bs through xmmap will map starting from page 0 in bs
// backing store can be shared as long as it is not being used as a private heap (can be shared through multiple calls to xmmap from different procs)
// can't call xmmap on same virtual address twice for same process
// a process can have a total of 8 mappings(virtual heap and xmmap), all must be in different stores
// can keep vheap pages in bs when they are accessed (don't need to move them to physical memory)
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  bs_map_t *bs = &bsm_tab[source];
  if(npages > bs->bs_npages)
    return SYSERR;
  int pd_offset = virtpage >> 10;
  int pt_offset = virtpage & 0x000003ff;
  int pid = getpid();
  struct pentry *pptr = &proctab[pid];
  struct vmblock *vmemlist = pptr->vmemlist;
  unsigned long vaddr = getvmem(vmemlist, virtpage, npages);
  bsm_map(pid, virtpage, source, npages);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  int pid = getpid();
  long vaddr = virtpage << 16;
  int pageth = 0, source = 0;
  bsm_lookup(pid, vaddr, &source, &pageth);
  if(source == -1) // source not mapped
    return SYSERR;
  bsm_unmap(pid, virtpage, 0);
  return OK;
}

unsigned long getvmem(struct vmblock *vmemlist, int virtpage, int npages)
{
    int i, end, vpno;
    int bad_req = 0, avail = -1;
    for(i = 0; i < 8; i++) {
        if(vmemlist[i].start > 0) {
            vpno = vmemlist[i].start >> 12;
            end = vpno + npages;
            if(virtpage >= vpno && virtpage <= end) {
                bad_req = -1;
                break;
            }
        } else {
            avail = i;
        }
    }
    if(bad_req == -1 || avail == -1) {
        return -1;
    }
    struct vmblock *vmb = &vmemlist[avail];
    vmb->start = virtpage << 12;
    vmb->npages = npages;
    return vmb->start;
}

/*
unsigned long getvmem(struct mblock *vmemlist, int virtpage, int npages) {
    struct    mblock    *p, *q, *leftover, *leftover_r;
    
    if (vmemlist->mnext == (struct mblock *) NULL) {
        return( -1);
    }
    int curr_vpn;
    int curr_end;
    int end = virtpage + npages;
    int nbytes = npages * NBPG;
    for (q= vmemlist,p=vmemlist->mnext; p != (struct mblock *) NULL; q=p,p=p->mnext) {
        curr_vpn = p;
        curr_vpn = curr_vpn >> 12;
        curr_end = curr_vpn + p->mlen; // might not be able to dereference this, will try to use page tables
        if(curr_vpn == virtpage && curr_end == end) {
            q->mnext = p->mnext;
            return (unsigned long)p;
        } else if(curr_vpn == virtpage && curr_end > end) { // leftover is to right of p, remove p
            leftover = (struct mblock *)(p->addr + nbytes);
            q->mnext = leftover;
            leftover->mnext = p->mnext;
            leftover->mlen = p->mlen - npages;
            return( (unsigned long)p->addr );
        } else if(curr_vpn < virtpage && curr_end > end) {
            leftover_r = (struct mblock *)((unsigned)end*NBPG);
            leftover_r->mnext = p->mnext;
            leftover_r->mlen = curr_end - end;
            p->mlen = virtpage - curr_vpn;
            p->mnext = leftover_r;
            struct mblock *block = (struct mblock *)((unsigned)virtpage*NBPG);
            block->mlen = npages;
            return((unsigned long)block->addr);
        }
    }
    return -1;
}
*/
