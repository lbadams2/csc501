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
  frm_tab[NFRAMES]; // this needs to be in the kernel
	int i;
	// pt in gpts[1] has the 1024 free frames
  int vpno = 0; // this should be upper 20 bits, offset into pd concat with offset into pt
                   // val doesn't matter when frame in unmapped
	struct *pt_t free_pt = gpts[1];
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
    if(grpolicy() == SC)
      avail = sc_repl_frm();
    else {
      
    }
    free_frm(avail);
  } else
      avail = i;
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  frm = &frm_tab[i]; 
  //get vp from frame, 20 bits
  int vpn = frm->fr_vpno;
  unsigned long vaddr_long = vpn << 12;
  virt_addr_t vaddr = (virt_addr_t)vaddr_long;
  struct pentry *pptr = &proctab[frm->fr_pid];
  struct pd_t *pd = (struct pd_t *)pptr->pdbr;
  pd = pd + vaddr.pd_offset; // pde we need
  unsigned long pt_addr = (unsigned long)pd->pd_base; // pd base is 20 bits (first location of page table, lower 12 bits 0)
                                                        // page tables must be placed at addresses divisible by page size to make this possible
  pt_addr = pt_addr << 12;
  struct pt_t *pt = (struct pt_t *) pt_addr;
  pt = pt + vaddr.pt_offset;
  pt->pt_pres = 0;
  frm->fr_refcnt--;
  if(frm->refcnt == 0)
    pd->pd_pres = 0;
  if(pt->pt_dirty == 1) {
      int store, pg_offset;
      int ret = bsm_lookup(frm->fr_pid, vaddr_long, &store, &pg_offset);
      if(ret == SYSERR) {
        kill(frm->frm_pid);
        return SYSERR;
      }
      write_bs((char *)pt, store, pg_offset);
  }
  return OK;
}