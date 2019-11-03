#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

bs_map_t bsm_tab[8];

// need to figure out relationship between bs and bs map
int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
  if(npages == 0 || npages > 256)
    return(SYSERR); 
  bs_map_t *bs = &bsm_tab[bs_id];
  int pid = getpid();
  if(bs->bs_status[pid] == BSM_MAPPED)
    npages = bs->bs_npages[pid];
  else {
    bs->bs_pid[pid] = 1;
    bs->bs_status[pid] = BSM_MAPPED;
    //bs->bs_vpno = vpno;
    bs->bs_npages[pid] = npages;    
  }

  return npages;
}


