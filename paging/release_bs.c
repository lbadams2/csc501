#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
  bs_map_t *bs = &bsm_tab[bs_id];
  int pid = getpid();
  bs->bs_status[pid] = BSM_UNMAPPED;
  bs->bs_npages[pid] = 0;
  bs->bs_pid[pid] = 0;
  return OK;

}

