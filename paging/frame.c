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
	// pt in gpts[2] has the 1024 free frames
	struct *pt_t free_pt = gpts[2];
	// first page is used by null proc
	fr_map_t *cur_inv_ent = &frm_tab[0];
	cur_inv_ent->fr_status = FRM_MAPPED;
	cur_inv_ent->fr_pid = 0;
	// difference between frame number and vpno? free_pt is pointing to first page entry in free_pt
	// pt_base is vpno of first page table entry
	cur_inv_ent->fr_vpno = free_pt->pt_base;
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
  kprintf("To be implemented!\n");
  return OK;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{

  kprintf("To be implemented!\n");
  return OK;
}



