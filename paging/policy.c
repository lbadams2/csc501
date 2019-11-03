/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <paging.h>


extern int page_replace_policy;
scq_t    *scq;
int	agq_head, agq_tail;
fr_map_t frm_tab[NFRAMES];
struct	qent agq[NFRAMES + 2];

void init_scq() {
  scq->capacity = NFRAMES;
  scq->front = 0;
  scq->back = -1;
  scq->size = 0;
}

void init_agq() {
  struct	qent	*hptr;
	struct	qent	*tptr;
	
	hptr = &agq[ agq_head=NFRAMES]; /* assign and rememeber queue	*/
	tptr = &agq[ agq_tail=NFRAMES + 1]; /* index values for head&tail	*/
	hptr->qnext = agq_tail;
	hptr->qprev = EMPTY;
	hptr->qkey  = MININT;
	tptr->qnext = EMPTY;
	tptr->qprev = agq_head;
	tptr->qkey  = MAXINT;
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

void ag_insert(int frm, int key)
{
	int	next;			/* runs through list		*/
	int	prev;

	next = agq[agq_head].qnext;
	while (agq[next].qkey < key)	/* tail has maxint as key	*/
		next = agq[next].qnext;
	agq[frm].qnext = next;
	agq[frm].qprev = prev = agq[next].qprev;
	agq[frm].qkey  = key;
	agq[prev].qnext = frm;
	agq[next].qprev = frm;
}

int ag_dequeue_frm(int i) {
  struct	qent	*mptr;		/* pointer to q entry for item	*/
	mptr = &q[i];
	agq[mptr->qprev].qnext = mptr->qnext;
	agq[mptr->qnext].qprev = mptr->qprev;
	return(i);
}

int ag_get_min()
{
	//struct	qent	*mptr;		/* pointer to q entry for item	*/
  int min_frm = agq[agq_head].qnext;
  fr_map_t *frm = &frm_tab[min_frm];
  // don't replace page tables and directories, remove them from q
  while(frm->fr_type != 0) {
    ag_dequeue_frm(min_frm);
    min_frm = agq[agq_head].qnext;
    frm = &frm_tab[min_frm];
  }
	return(min_frm);
}

void agq_adjust_keys() {
  int next = agq[agq_head].qnext;
  struct	qent	*mptr;
  fr_map_t *frm;
  while(next != -1) {
    mptr = &agq[next];
    mptr->qkey = mptr->qkey >> 1;
    frm = &frm_tab[next];
    int ref_bit = get_pgref_bit(frm);
    if(ref_bit == 1) {
      mptr->qkey = mptr->qkey + 128;
      if(mptr->qkey > 255)
        mptr->qkey = 255;
    }
  }
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

int sc_dequeue() {
  if(scq->size == 0) {
    kprintf("scq underflow");
    return -1;
  }
  scq->front = (scq->front + 1) % scq->capacity;
  scq->size--;
  return scq->front;
}

int sc_front() {
  if(scq->size == 0) {
    kprintf("scq underflow");
    return -1;
  }
  return scq->frames[scq->front];
}

int sc_repl_frm() {
  int front = sc_front();
  fr_map_t *frm = &frm_tab[front];
  int ref_bit = get_pgref_bit(frm);
  if(!ref_bit && frm->fr_type == 0) // don't replace page tables and directories
    return scq->frames[front];
  int pos = (scq->front + 1) % scq->capacity;
  //if(pos < 0)
  //  pos = pos + NFRAMES;
  //pos = pos % scq->capacity;
  int num_visited = 1;
  while(pos != front && num_visited < scq->size) {
    frm = &frm_tab[pos];
    ref_bit = get_pgref_bit(frm);
    if(!ref_bit && frm->fr_type == 0) // don't replace page tables and directories
      return scq->frames[pos];
    pos = (pos + 1) % scq->capacity;
    num_visited++;
    //int pos = pos - 1;
    //if(pos < 0)
    //  pos = pos + NFRAMES;
    //pos = pos % scq->capacity;
  }
  frm = &frm_tab[front];
  while(frm->fr_type != 0) // don't replace page tables and directories
    sc_dequeue();
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
