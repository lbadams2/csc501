#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
  bs_map_t *bs = &bsm_tab[bs_id];
  bs->bs_status = BSM_UNMAPPED;
  bs->bs_npages = 0;
  bs->bs_pid = -1;
  return OK;

}

