/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
extern int pferrcode;
SYSCALL pfint()
{
  // CR2 register has the address that generated the exception
  unsigned long addr = read_cr2();
  virt_addr_t vaddr = (virt_addr_t)addr;
  int pid = getpid();
  struct pentry *pptr = &proctab[pid];
  struct pd_t *pd = pptr->pdbr;
  unsigned int pd_offset = vaddr.pd_offset;
  struct pd_t *pde = pd + pd_pffset; // address of pde
  unsigned int pt_offset = vaddr.pt_offset;
  // if address hasn't been mapped in pd return an error
  int avail;
  struct pt_t *pt;
  if(pde->pd_pres == 0) {
    // get frame for page table
    get_frm(&avail);
    fr_map_t *frm = &frm_tab[avail];
    frm->fr_status = FRM_MAPPED;
    frm->fr_pid = pid;
    frm->fr_refcnt = 1;
    frm->fr_type = FR_TBL;
    frm->fr_dirty = 0;
    unsigned int vpno = pd_offset << 10;
    vpno = vpno | pt_offset;
    frm->fr_vpno = vpno;
    pt = create_page_table(avail);
    pde->pd_pres = 1;
    pde->pd_base = pt; // address of page table
  } else
      pt = (struct pt_t *)pde->pt_base; // address of page table
  
  int store, page;
  bsm_lookup(pid, vaddr, &store, &page);
  // get frame for page
  get_frm(&avail);
  char *frm_phy_addr = (char *)avail*NBPG;
  // copy page from bs into memory
  read_bs(frm_phy_addr, store, page);
  pt = pt + pt_offset; // address of pte
  pt->pt_base = avail * NBPG; // address of page
  pt->pt_pres = 1;
  pt->pt_write = 1;
  pt->pt_dirty = 0;
  pt->pt_acc = 1;
  pt->pt_global = 0;
  pt->pt_mbz = 0;
  return OK;
}

struct pt_t *create_page_table(int frm_no) {
	int i;
  unsigned long frm_addr = frm_no * NBPG;
  struct pt_t *pt = (struct pt_t *)frm_addr;
	//struct pt_t *pt =  (struct pt_t *)getmem(sizeof(struct pt_t) * 1024); // this address needs to be divisible by NBPG
	//unsigned long bs_base_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + (pt_ix * NBPG * 1024);
	for(i = 0; i < 1024; i++) {
		pt->pt_pres = 1;
		pt->pt_write = 1;
		pt->pt_user = 0;
		pt->pt_pwt = 0;
		pt->pt_pcd = 0;
		pt->pt_acc = 0;
		pt->pt_dirty = 0;
		pt->pt_mbz = 0;
		pt->pt_global = 0;
		pt->pt_avail = 0;
		
		//unsigned int bs_phy_addr = bs_base_addr + i*NBPG;
		//pt->pt_base = bs_phy_addr; // this should be location in backing store or in free frames
								   // top 20 bits, should be in address divisible by page size (all frames are located in such an address) 
		int avail;
		// global page tables cover frames 0 - 4095, get_frm would return frame between 1024 and 2047
		// pt base needs to be address of page in physical memory, offset added to get specific address
		//int ret = get_frm(&avail);
		//pt->pt_base = avail * NBPG; // address of frame (page)
		pt->pt_base = NULL;
		pt++;
	}
	return pt - 1024;
}