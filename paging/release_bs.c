#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
    free_bsm(bs_id);
    // may need to also release the virtual memory
   return OK;

}

