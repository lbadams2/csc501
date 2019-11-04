#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

bs_map_t bsm_tab[8];

// bs cannot be shared if its being used as a private heap
// initially backing stores don't exist, and don't have to be 256 pages, but they will start at the boundaries defined in paging.h and proj spec
int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
  if(npages == 0 || npages > 256)
    return(SYSERR);
  bs_map_t *bs = &bsm_tab[bs_id];
  int pid = getpid();
  if(bs->bs_status == BSM_MAPPED)
    npages = bs->bs_npages[pid];
  else {
    bs->bs_pid] = pid;
    bs->bs_status = BSM_MAPPED;
    //bs->bs_vpno = vpno;
    bs->bs_npages = npages;    
  }
  return npages;
}


