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
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
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
  kprintf("To be implemented!");
  return SYSERR;
}


unsigned long getvmem(struct vmblock *vmemlist, int virtpage, int npages)
{
    int i, end, vpno;
    int bad_req = 0, avail = -1;
    for(i = 0; i < 8; i++) {
        if(vmemlist[i].start > 0) {
            vpno = vmemlist[i].start >> 12;
            end = vpno + npages;
            if(virtpage >= vpno && virtpage <= end) { // virtpage already being used by process
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