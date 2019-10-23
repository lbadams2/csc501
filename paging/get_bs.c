#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
  if(npages == 0 || npages > 256)
    return(SYSERR); 
  unsigned long = BACKING_STORE_BASE;
  //check if bs with bs_id exists
  //create new if not, return err if can't create new
  return npages;

}


