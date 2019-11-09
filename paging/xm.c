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
  //int pd_offset = virtpage >> 10;
  //int pt_offset = virtpage & 0x000003ff;
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
  long vaddr = virtpage << 12;
  int pageth = 0, source = 0;
  bsm_lookup(pid, vaddr, &source, &pageth);
  if(source == -1) // source not mapped
    return SYSERR;
  bsm_unmap(pid, virtpage, 0);
  struct vmblock *mptr;
  int i;
  struct pentry *pptr = &proctab[pid];
	for(i = 0; i < 8; i++) {
		mptr = &pptr->vmemlist[i];
		vfreemem(mptr, mptr->npages);
	}
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