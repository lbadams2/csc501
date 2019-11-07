/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


pt_t *create_page_table(int);
/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
extern int pferrcode;
SYSCALL pfint()
{
  kprintf("***************** in pfint *******************\n");
  // CR2 register has the address that generated the exception
  unsigned long addr = read_cr2();
  kprintf("got addr from cr2 %d\n", addr);
  unsigned int pd_offset = addr >> 22;
  kprintf("pd offset %d\n", pd_offset);
  //pd_offset = pd_offset << 2;
  //virt_addr_t vaddr = (virt_addr_t)addr;
  int pid = getpid();
  struct pentry *pptr = &proctab[pid];
  pd_t *pd = (pd_t *)pptr->pdbr;
  kprintf("pfint pd addr %d\n", pd);
  int store, page;
  bsm_lookup(pid, addr, &store, &page);
  kprintf("bsm lookup store %d page %d\n", store, page);
  if(store  == -1) { // illegal, hasn't been mapped to bs
    kprintf("address hasn't been mapped to backing store");
    kill(getpid());
  }

  //unsigned int pd_offset = vaddr.pd_offset;
  pd_t *pde = pd + pd_offset; // address of pde
  unsigned int pt_offset = addr >> 12;
  pt_offset = pt_offset & 0x000003ff;
  kprintf("pt offset %d\n", pt_offset);
  //pt_offset = pt_offset << 2;
  //unsigned int pt_offset = vaddr.pt_offset;
  // if address hasn't been mapped in pd return an error
  int avail;
  pt_t *pt;
  if(pde->pd_pres == 0) {
    kprintf("pfint page table not present\n");
    // get frame for page table
    get_frm(&avail);
    fr_map_t *frm = &frm_tab[avail];
    frm->fr_status = FRM_MAPPED;
    frm->fr_pid = pid;
    frm->fr_refcnt = 1;
    frm->fr_type = FR_TBL;
    frm->fr_dirty = 0;
    kprintf("pt in frame %d\n", avail + FRAME0);
    frm->fr_vpno = avail + FRAME0;
    pt = create_page_table(avail);
    kprintf("pfint address of new pt %d\n", pt);
    pde->pd_pres = 1;
    unsigned int test = (unsigned int)pt;
    test = test >> 12;
    kprintf("pt base is %d", test);
    pde->pd_base = test; // address of page table
  } else {
      unsigned int pt_addr = pde->pd_base << 12;
      pt = (pt_t *)pt_addr; // address of page table
  }
  
  // set pt base to physical frame number of backing store
  int bs_frame = (store *256) + 2048 + page;
  kprintf("frame number of bs %d\n", bs_frame);
  pt = pt + pt_offset; // address of pte
  kprintf("pt base %d\n", pt);
  pt->pt_base = bs_frame; // address of page
  pt->pt_pres = 1;

  // get frame for page
  //get_frm(&avail);
  //int bs_addr = avail*NBPG;
  //char *frm_phy_addr = (char *)bs_addr;
  // copy page from bs into memory
  //read_bs(frm_phy_addr, store, page);
  //pt = pt + pt_offset; // address of pte
  //pt->pt_base = avail * NBPG; // address of page
  //pt->pt_pres = 1;
  return OK;
}

pt_t *create_page_table(int frm_no) {
	int i;
  unsigned long frm_addr = (frm_no + FRAME0) * NBPG;
  pt_t *pt = (pt_t *)frm_addr;
	//struct pt_t *pt =  (struct pt_t *)getmem(sizeof(struct pt_t) * 1024); // this address needs to be divisible by NBPG
	//unsigned long bs_base_addr = BACKING_STORE_BASE + bs_id*BACKING_STORE_UNIT_SIZE + (pt_ix * NBPG * 1024);
	for(i = 0; i < 1024; i++) {
		pt->pt_pres = 0;
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
                   
		// global page tables cover frames 0 - 4095, get_frm would return frame between 1024 and 2047
		// pt base needs to be address of page in physical memory, offset added to get specific address
		//int ret = get_frm(&avail);
		//pt->pt_base = avail * NBPG; // address of frame (page)
		pt->pt_base = NULL;
		pt++;
	}
	return pt - 1024;
}
