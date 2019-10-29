/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


extern int page_replace_policy;

void init_scq() {
  scq->frames[NFRAMES];
  scq->capacity = NFRAMES;
  scq->front = 0;
  scq->back = -1;
  scq->size = 0;
}

// back will increment and front 
void sc_enqueue(int frm) {
  if(scq->size == scq->capacity) {
    kprintf("scq overflow");
    return;
  }
  scq->back = (scq->back + 1) % scq->capacity;
  scq->frames[scq->back] = frm;
  scq->size++;
}

void sc_dequeue() {
  if(scq->size == 0) {
    kprintf("scq underflow");
    return;
  }
  scq->front = (scq->front + 1) % scq->capacity;
  scq->size--;
}

int sc_front() {
  if(scq->size == 0) {
    kprintf("scq underflow");
    return;
  }
  return scq->frames[scq->front];
}

int get_pgref_bit(struct fr_map_t *frm) {
  int pid = frm->fr_pid;
  int vpno = frm->fr_vpno;
  struct pentry *pptr = &proctab[pid];
  struct pd_t *pd = (struct pd_t *)pptr->pdbr;
  int pd_offset = (vpno >> 10) << 10;
  pd = pd + pd_offset;
  struct pt_t *pt = (struct pt_t *)pd->pd_base;
  int pt_offset = vpno & 0x000003ff;
  pt = pt + pt_offset;
  int ref_bit = pt->pt_acc;
  if(ref_bit)
    pt->pt_acc = 0;
  return ref_bit;
}

int sc_repl_frm() {
  int front = sc_front();
  struct fr_map_t *frm = &frm_tab[front];
  int ref_bit = get_pgref_bit(frm);
  if(!ref_bit)
    return scq->frames[front];
  int pos = (scq->front + 1) % scq->capacity;
  //if(pos < 0)
  //  pos = pos + NFRAMES;
  //pos = pos % scq->capacity;
  int num_visited = 1;
  while(pos != front && num_visited < scq->size) {
    frm = &frm_tab[pos];
    ref_bit = get_pgref_bit(frm);
    if(!ref_bit)
      return scq->frames[pos];
    pos = (pos + 1) % scq->capacity;
    num_visited++;
    //int pos = pos - 1;
    //if(pos < 0)
    //  pos = pos + NFRAMES;
    //pos = pos % scq->capacity;
  }
  return scq->frames[front];
}
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  page_replace_policy = policy;
  if(policy == SC)
    init_scq();
  else {

  }
  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}
