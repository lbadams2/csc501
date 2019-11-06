#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {
  /* requests a new mapping of npages with ID map_id */
    if(npages == 0 || npages > 256)
      return(SYSERR);

    bs_map_t *bs = &bsm_tab[bs_id];
    if(bs->bs_status == BSM_MAPPED)
      npages = bs->bs_npages;
      
    return npages;
}


