/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


extern int page_replace_policy;
sc_qent_t scq[NFRAMES];
fr_map_t frm_tab[NFRAMES];
int scq_head;
int scq_tail;
int scq_size;

void init_scq() {
    int i;
    scq_head = -1;
    scq_tail = -1;
    scq_size = 0;
    sc_qent_t *current;
    for(i = 0; i < NFRAMES; i++) {
        current = &scq[i];
        current->qnext = -1;
        current->qprev = -1;
    }
}


void ag_insert(int frm, int key) {

}

int ag_dequeue_frm(int i) {
  return 0;
}

int ag_get_min() {
  return 0;
}

void agq_adjust_keys() {

}

int get_pgref_bit(fr_map_t *frm) {
  int pid = frm->fr_pid;
  int vpno = frm->fr_vpno;
  struct pentry *pptr = &proctab[pid];
  pd_t *pd = (pd_t *)pptr->pdbr;
  int pd_offset = (vpno >> 10) << 10;
  pd = pd + pd_offset;
  pt_t *pt = (pt_t *)pd->pd_base;
  int pt_offset = vpno & 0x000003ff;
  pt = pt + pt_offset;
  int ref_bit = pt->pt_acc;
  if(ref_bit && page_replace_policy == SC)
    pt->pt_acc = 0;
  return ref_bit;
}

// need to work on this later
int sc_dequeue_frm(int frm) {
  sc_qent_t *prev, *next, *current;
  current = &scq[frm];
  next = &scq[current->qnext];
  prev = &scq[current->qprev];
  next->qprev = current->qprev;
  prev->qnext = current->qnext;
  if(frm == scq_head) {
    scq_head = current->qprev;
  }
  if(frm == scq_tail) {
    scq_tail = current->qnext;
  }
  current->qnext = -1;
  current->qprev = -1;
  return frm;
}

int sc_replace() {
    int pos = scq_head;
    sc_qent_t *current;
    fr_map_t *frm;
    do {
        current = &scq[pos];
        frm = &frm_tab[pos];
        if(frm->fr_type != 0)
          continue;
        int ref_bit = get_pgref_bit(frm); // this clears bit if its 1
        if(ref_bit == 0) {
            scq[scq_head].qprev = pos;
            scq_head = pos;
            scq[scq_head].qprev = scq_tail;
            scq[scq_tail].qnext = scq_head;
            return pos; // frame number being evicted
        }
        pos = current->qnext;
    } while(pos != scq_head);
    
    return scq_head;
}

int sc_dequeue() {
    sc_qent_t *current = &scq[frm];
    if(scq_size == 0)
        return -1;
    else if(scq_size == 1) {
        current->qnext = -1;
        current->qprev = -1;
        scq_size--;
        scq_head = -1;
        scq_tail = -1;
        return frm;
    }
    scq[current->qprev].qnext = current->qnext;
    scq[current->qnext].qprev = current->qprev;
    if(frm == scq_head)
        scq_head = current->qnext;
    if(frm == scq_tail)
        scq_tail = current->qprev;
    current->qnext = -1;
    current->qprev = -1;
    scq_size--;
    return frm;
}

void sc_enqueue(int frm) {
    sc_qent_t *prev;
    sc_qent_t *current = &scq[frm];
    if(scq_size == 0) {
        scq_head = frm;
        scq_tail = frm;
        scq_size++;
        return;
    }
    scq_size++;
    prev = &scq[scq_tail];
    current->qprev = scq_tail;
    prev->qnext = frm;
    current->qnext = scq_head;
    scq_tail = frm;
    scq[scq_head].qprev = scq_tail;
    scq[scq_tail].qnext = scq_head;
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
