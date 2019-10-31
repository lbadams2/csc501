/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <paging.h>
#include <stdio.h>

void release_vmem();

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	release_vmem(pid, pptr);
	restore(ps);
	return(OK);
}

void release_vmem(int pid, struct pentry *pptr) {
	// get mapped pages from bsm map
	frm_map_t *frm;
	for(i = 0; i < NFRAMES; i++) {
		frm = &frm_tab[i];
		if(frm->fr_pid == pid)
			free_frm(i); // this will also free page directory, maybe need to save for later to complete below steps
	}
	for(i = 0; i < 8; i++) {
		bs_map_t *bs = &bsm_tab[source];
		if(bs->bs_pid[pid] == 1) {
			bsm_unmap(pid, bs->bs_vpno[pid], 0);
		}
	}
	// release bs, maybe using free_frm from global page table, should use get_frm in get_bs
	struct mblock *mptr = pptr->vmemlist;
	while(mptr) {
		vfreemem(mptr, mptr->mlen);
		mptr->mnext;
	}
}