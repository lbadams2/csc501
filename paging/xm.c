/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  int pid = getpid();
  long vaddr = virtpage << 16;
  int pageth = 0;
  bsm_lookup(pid, vaddr, &source, &pageth);
  if(source != -1) // source already mapped
    return SYSERR;
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
  int pageth = 0;
  bsm_lookup(pid, vaddr, &source, &pageth);
  if(source == -1) // source not mapped
    return SYSERR;
  bsm_unmap(pid, virtpage, 0);
  return OK;
}
