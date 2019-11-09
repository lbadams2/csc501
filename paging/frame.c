/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

void invalidate_frm(int);
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
  for(i = 0; i < NFRAMES; i++) {
    frm = &frm_tab[i];
    if(frm->fr_status == FRM_UNMAPPED)
      break;
  }
  // no free frames, replace one
  if(i == NFRAMES) {
    // if frame belongs to current process call invlpg instruction
    if(grpolicy() == SC) {
      *avail = sc_replace();
      invalidate_frm(*avail);
      //sc_enqueue(avail);
    }
    else {
      agq_adjust_keys();
      *avail = ag_get_min();
      invalidate_frm(*avail);
    }
    return(OK);
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

void remove_ipt(int i) {
  if(grpolicy() == SC)
      sc_dequeue(i);
  else
      ag_dequeue(i);

  fr_map_t *frm = &frm_tab[i];
  frm->fr_status = FRM_UNMAPPED;
  frm->fr_pid = NULL;
  frm->fr_vpno = NULL;
  frm->fr_refcnt = 0;
  frm->fr_dirty = 0;
}

void invalidate_frm(int i) {
  fr_map_t *frm = &frm_tab[i]; 
  //get vp from frame, 20 bits
  int vpn = frm->fr_vpno;
  int pd_offset = vpn >> 10;
  int pid = frm->fr_pid;
  struct pentry *pptr = &proctab[pid];
  pd_t *pd = (pd_t *)pptr->pdbr;
  pd = pd + pd_offset;
  pt_t *pt = (pt_t *)pd->pd_base;
  int pt_offset = vpn & 0x000003ff;
  
  int pt_frmno = (unsigned long)pt >> 12;
  pt_frmno = pt_frmno - FRAME0;
  fr_map_t *pt_frm = &frm_tab[pt_frmno];

  pt = pt + pt_offset;
  pt->pt_pres = 0;
  int curr_pid = getpid();
  if(curr_pid == frm->fr_pid) {
    unsigned long addr = (unsigned long) frm;
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
  }
  pt_frm->fr_refcnt--;
  if(pt_frm->fr_refcnt == 0) {
    pd->pd_pres = 0;
    remove_ipt(pt_frmno);
  }
  if(pt->pt_dirty == 1) {
      int store, pg_offset;
      //int ret = bsm_lookup(frm->fr_pid, vaddr_long, &store, &pg_offset);
      // to free whole page can use vpn, don't need page offset
      int ret = bsm_lookup(frm->fr_pid, vpn*NBPG, &store, &pg_offset);
      if(ret == SYSERR) {
        kill(frm->fr_pid);
        return SYSERR;
      }
      unsigned long pa = (unsigned long)pt->pt_base;
      pa = pa << 12;
      char *phys_addr = (char *)pa;
      write_bs(phys_addr, store, pg_offset);
      pt->pt_dirty = 0;
      // next attempt will be page fault, bsm lookup will get it back from bs
  }
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  int store, pg_offset;
  fr_map_t *frm = &frm_tab[i];
  int vpn = frm->fr_vpno;
  int pt_offset = vpn & 0x000003ff;
  int pd_offset = vpn >> 10;
  struct pentry *pptr = &proctab[frm->fr_pid];
  pd_t *pd = (pd_t *)pptr->pdbr;
  pd = pd + pd_offset;  
  pt_t *pt = (pt_t *)pd->pd_base;
  pt = pt + pt_offset;
  unsigned long pa = (unsigned long)pt->pt_base;
  pa = pa << 12;
  char *phys_addr = (char *)pa;
  int ret = bsm_lookup(frm->fr_pid, vpn*NBPG, &store, &pg_offset);
  write_bs(phys_addr, store, pg_offset);
  pt->pt_dirty = 0;
  invalidate_frm(i);
  remove_ipt(i);
  return OK;
}



