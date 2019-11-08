/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


extern int page_replace_policy;
sc_qent_t scq[NFRAMES];
agq_ent_t agq[NFRAMES];
fr_map_t frm_tab[NFRAMES];
int scq_head;
int scq_tail;
int scq_size;
int agq_head;
int agq_tail;
int agq_size;

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

void init_agq() {
    int i;
    agq_ent_t *current;
    for(i = 0; i < NFRAMES; i++) {
        current = &agq[i];
        current->qnext = -1;
        current->qprev = -1;
        current->qkey = 255;
        agq_head = -1;
        agq_tail = -1;
    }
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


// key should always be 0
void ag_insert(int frm, int key)
{
    agq_ent_t *current = &agq[frm];
    agq_ent_t *prev = &agq[agq_tail];
    if(agq_size == 0) {
        agq_head = frm;
        agq_tail = frm;
        current->qkey = key;
        agq_size++;
        return;
    }
    prev->qnext = frm;
    current->qprev = agq_tail;
    current->qnext = -1;
    current->qkey = key;
    agq_tail = frm;
    agq_size++;
    /*
    next = agq_head;
    prev = -1;
    while (agq[next].qkey > key) {
        prev = next;
        next = agq[next].qnext;
    }
    agq[frm].qnext = next;
    agq[frm].qprev = prev;
    agq[frm].qkey  = key;
    if(prev != -1)
        agq[prev].qnext = frm;
    else
        agq_head = frm;
    if(next != -1)
        agq[next].qprev = frm;
    else
        agq_tail = frm;
    */
}

int ag_dequeue(int i) {
    agq_ent_t    *mptr, *next, *prev;        /* pointer to q entry for item    */
    mptr = &agq[i];
    if(i == agq_head) {
        next = &agq[mptr->qnext];
        next->qprev = -1;
        agq_head = mptr->qnext;
    }
    else
        agq[mptr->qprev].qnext = mptr->qnext;
    if(i == agq_tail) {
        prev = &agq[mptr->qprev];
        prev->qnext = -1;
        agq_tail = mptr->qprev;
    }
    else
        agq[mptr->qnext].qprev = mptr->qprev;
    mptr->qkey = 255;
    mptr->qnext = -1;
    mptr->qprev = -1;
    agq_size--;
    return(i);
}

int ag_get_min()
{
    if(agq_head == -1)
        return -1;
    int min_frm = agq_tail;
    fr_map_t *frm = &frm_tab[min_frm];
    // don't replace page tables and directories
    while(frm->fr_type != 0) {
        min_frm = agq[min_frm].qprev;
        frm = &frm_tab[min_frm];
    }
    return(min_frm);
}

void agq_adjust_keys() {
    int current = agq_head;
    agq_ent_t *mptr;
    fr_map_t *frm;
    while(current != -1) {
        mptr = &agq[current];
        mptr->qkey = mptr->qkey >> 1;
        frm = &frm_tab[current];
        int ref_bit = get_pgref_bit(frm);
        if(ref_bit == 1) {
            mptr->qkey = mptr->qkey + 128;
            if(mptr->qkey > 255)
                mptr->qkey = 255;
        }
    }
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

int sc_dequeue(int frm) {
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
