#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

// need to figure out relationship between bs and bs map
int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
  if(npages == 0 || npages > 256)
    return(SYSERR); 
  bs_map_t *bs = &bsm_tab[bs_id];
  if(bs->bs_status == BSM_MAPPED)
    npages = bs->bs_npages;
  else {
    //bs->bs_pid = pid;
    bs->bs_status = BSM_MAPPED;
    //bs->bs_vpno = vpno;
    bs->bs_npages = npages;    
  }

  return npages;
}


