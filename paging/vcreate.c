/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
WORD *getvhp(int hsize);
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	int pid = create(procaddr, ssize, priority, name, nargs, args);
	STATWORD 	ps;
	disable(ps);
	struct	pentry	*pptr = &proctab[pid];
	if((vaddr = (unsigned long *)getvhp(hsize)) == (unsigned long *)SYSERR) {
		restore(ps);
		return(SYSERR);
	}
	struct *pd_t pd = create_page_dir();
	pptr->vhpnpages = hsize;
	// starting page no for heap
	int avail = 0;
	int ret = get_bsm(&avail);
	if(ret == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	ret = bsm_map(pid, 0, avail, hsize);
	if(ret == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	pptr->vhpno = 0; // set when page table created, not sure what to put here
	pptr->store = avail; // set when page table created
	pptr->pdbr = pd;
	restore(ps);
	return(pid);
}

// mblock->mlen starts as difference between top address and bottom address in free frames (# of bytes difference)
WORD *getvhp(int hsize) {
	struct	mblock	*p, *q, *leftover;
	unsigned int nbytes = hsize * NBPG;
	if (nbytes==0 || memlist.mnext== (struct mblock *) NULL) {
		return( (WORD *)SYSERR);
	}
	nbytes = (unsigned int) roundmb(nbytes);
	for (q= &vmemlist,p=vmemlist.mnext ;
	     p != (struct mblock *) NULL ;
	     q=p,p=p->mnext)
		// if block is exactly right size return it and remove it from list
		if ( p->mlen == nbytes) {
			q->mnext = p->mnext;
			return( (WORD *)p );
		} else if ( p->mlen > nbytes ) {
			// create new block starting from the memory chosen block leaves off at
			leftover = (struct mblock *)( (unsigned)p + nbytes );
			q->mnext = leftover;
			leftover->mnext = p->mnext;
			leftover->mlen = p->mlen - nbytes;
			return( (WORD *)p );
		}
	return( (WORD *)SYSERR );
}

struct *pd_t create_page_dir() {
	struct *pd_t pd = (struct *pd_t)next_free_addr; // this should be in free frames
	null_pd->pd_pres = 0;
	null_pd->pd_write = 1;
	null_pd->pd_user = 0;
	null_pd->pd_pwt = 0;
	null_pd->pd_pcd = 0;
	null_pd->pd_acc = 0;
	null_pd->pd_mbz = 0;
	null_pd->pd_fmb = 0;
	null_pd->pd_global = 0;
	null_pd->pd_avail = 0;
	// null proc uses first free frame in page table representing free frames (frames 1024-2047)
	null_pd->pd_base = NULL;
	next_free_addr++;
	return pd;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}


/*
WORD *getvhp(int hsize) {
	struct	mblock	*p, *q;	
	struct	mblock	*fits, *fitsq=NULL;
	unsigned int nbytes = hsize * NBPG;
	WORD	len;
	nbytes = (unsigned int) roundew(nbytes);
	fits = (struct mblock *) NULL;
	q = &vmemlist;
	// q follows p along vmemlist as long as p is not NULL
	// if size of p is >= nbytes set fits and its preceding block fitsq
	for (p = q->mnext ; p !=(struct mblock *) NULL ; q = p,p = p->mnext)
		if ( p->mlen >= nbytes) {
			fitsq = q;
			fits = p;
		}
	// no block fits nbytes
	if (fits == (struct mblock *) NULL) {
		return( (WORD *)SYSERR );
	}
	// if block fits nbytes exactly remove fits from the list
	if (nbytes == (len = fits->mlen) ) {
		fitsq->mnext = fits->mnext;
	} else { // else adjust bytes in fits
		fits->mlen -= nbytes;
	}
	// get address of top most WORD in fits
	//fits = (struct mblock *) ((WORD) fits + len - sizeof(WORD));
	// get address of first WORD in newly allocated block
	fits = (struct mblock *) ((WORD) fits + fits->mlen;
	// store nbytes in top most address of fits
	*((WORD *) fits) = nbytes;
	return( (WORD *) fits); // return address of bottom WORD
}
*/