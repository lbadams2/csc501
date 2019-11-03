/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

void add_frm_pt(fr_map_t *frm);

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  frm_tab[NFRAMES]; // this needs to be in the kernel
	int i;
	// pt in gpts[1] has the 1024 free frames
  int vpno = 0; // this should be upper 20 bits, offset into pd concat with offset into pt
                   // val doesn't matter when frame in unmapped
	pt_t *free_pt = gpts[1];
	// first page is used by null proc
	fr_map_t *cur_inv_ent = &frm_tab[0];
	cur_inv_ent->fr_status = FRM_MAPPED;
	cur_inv_ent->fr_pid = 0;
	// difference between frame number and vpno? free_pt is pointing to first page entry in free_pt
	// pt_base is vpno of first page table entry
	cur_inv_ent->fr_vpno = vpno;
	cur_inv_ent->fr_refcnt = 0;
	cur_inv_ent->fr_type = FR_PAGE;
	cur_inv_ent->fr_dirty = 0;
	for(i = 1; i < NFRAMES; i++) {
		cur_inv_ent = &frm_tab[i];
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
// need to add frame to page table, create pte
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
      avail = sc_repl_frm(); // doesn't dq
      free_frm(avail);
      //sc_enqueue(avail);
    }
    else {
      agq_adjust_keys();
      avail = ag_get_min();
      free_frm(avail);
    }
    
  } else
      avail = i;
    
  if(grpolicy() == SC)
    sc_enqueue(avail);
  else
    ag_insert(avail, 0);
  // maybe prevent page tables and page directories from getting replaced
  int frm_addr = (NFRAMES + *avail) * NBPG;
  frm = (fr_map_t *)frm_addr;
  add_frm_pt(frm);  
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
// need to remove frame from page table
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
  if(grpolicy() == SC)
      sc_dequeue(i);
  else
      ag_dequeue_frm(i);
  return OK;
}

void add_frm_pt(fr_map_t *frm) {
  int pid = getpid();
  struct pentry *pptr = &proctab[pid];
  pd_t *pd = (pd_t *)pptr->pdbr;
  pt_t *pt;
  int i, j, vpn = 0, added_frm = 0;
  for(i = 4; i < NFRAMES; i++) { 
    if(pd[i].pd_pres) {
      pt = (pt_t *)pd[i].pd_base;
      for(j = 0; i < NFRAMES; j++) { // find empty pte in existing page table
        if(pt[j].pt_pres == 0) {
          vpn = i << 10;
          vpn = vpn | j;
          pt[j].pt_pres = 1;
          pt[j].pt_write = 1;
          pt[j].pt_mbz = 0;
          pt[j].pt_dirty = 0; // might be dirty
          pt[j].pt_global = 0;
          unsigned int frame_addr = (unsigned int)frm;
          pt[j].pt_base = frame_addr; // physical address of frame (offset 0)
          added_frm = 1;
          break;
        }
      }
      if(added_frm == 1)
        break;
    }
    else { // create page table

    }
  }
  frm->fr_vpno = vpn;
}