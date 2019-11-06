/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  int i;
	for(i = 0; i < NFRAMES; i++) {
		fr_map_t *cur_inv_ent = &frm_tab[i];
		cur_inv_ent->fr_status = FRM_UNMAPPED;
		cur_inv_ent->fr_pid = NULL;
		cur_inv_ent->fr_vpno = NULL;
		cur_inv_ent->fr_refcnt = 0;
		cur_inv_ent->fr_type = 0;
		cur_inv_ent->fr_dirty = 0;
	}
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  int i;
  fr_map_t *frm;
  for(i = 0; i < 1024; i++) {
    frm = &frm_tab[i];
    if(frm->fr_status == FRM_UNMAPPED)
      break;
  }
  // no free frames, replace one
  if(i == 1024) {
    // if frame belongs to current process call invlpg instruction
    if(grpolicy() == SC) {
      *avail = sc_dequeue(); // does dq, need to work on it later
      free_frm(*avail);
      //sc_enqueue(avail);
    }
    else {
      agq_adjust_keys();
      *avail = ag_get_min();
      free_frm(*avail);
    }
    
  } else
      *avail = i;
    
  if(grpolicy() == SC)
    sc_enqueue(*avail);
  else
    ag_insert(*avail, 0);
  // maybe prevent page tables and page directories from getting replaced
  //int frm_addr = (NFRAMES + *avail) * NBPG;
  //frm = (fr_map_t *)frm_addr;
  //add_frm_pt(frm);
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  fr_map_t *frm = &frm_tab[i]; 
  //get vp from frame, 20 bits
  int vpn = frm->fr_vpno;
  int pd_offset = (vpn >> 10) << 10;
  int pid = frm->fr_pid;
  struct pentry *pptr = &proctab[pid];
  pd_t *pd = (pd_t *)pptr->pdbr;
  pd = pd + pd_offset;
  pt_t *pt = (pt_t *)pd->pd_base;
  int pt_offset = vpn & 0x000003ff;
  pt = pt + pt_offset;
  pt->pt_pres = 0;
  int curr_pid = getpid();
  if(curr_pid == frm->fr_pid) {
    unsigned long addr = (unsigned long) frm;
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
  }
  frm->fr_refcnt--;
  if(frm->fr_refcnt == 0)
    pd->pd_pres = 0;
  if(pt->pt_dirty == 1) {
      int store, pg_offset;
      //int ret = bsm_lookup(frm->fr_pid, vaddr_long, &store, &pg_offset);
      // to free whole page can use vpn, don't need page offset
      int ret = bsm_lookup(frm->fr_pid, vpn, &store, &pg_offset);
      if(ret == SYSERR) {
        kill(frm->fr_pid);
        return SYSERR;
      }
      write_bs((char *)pt, store, pg_offset);
  }
  // add this later
  /*
  if(grpolicy() == SC)
      sc_dequeue(i);
  else
      ag_dequeue_frm(i);
  */
  return OK;
}



