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
WORD *init_vmemlist(struct pentry *pptr, int npages);
struct pd_t *create_page_dir(int npages, int bs_id);
struct pt_t *create_page_table(int pt_ix, int bs_id);

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
	//if((vaddr = (unsigned long *)getvhp(hsize)) == (unsigned long *)SYSERR) {
	//	restore(ps);
	//	return(SYSERR);
	//}
	int vpno = 4 << 10;
	struct pd_t *pd = create_page_dir(avail, vpno, pid);
	pd = pd + 4; // skip over page tables for physical memory
	// create page table
	// page is 4 KB	
	int avail = 0;
	int ret = get_bsm(&avail); // get_bsm, then get_bs, then bsm_map
	if(ret == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	// need to use semaphore here
	// offset into pd concat with offset into pt, offset into pd is 4 to skip 0-4095, offset into pt is 0 because it doesn't exist yet
	int npages_ret = get_bs(avail, hsize); // get bs should come before bsm map
	ret = bsm_map(pid, vpno, avail, hsize);
	if(ret == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	init_vmemlist(pptr->vmemlist, vpno, hsize);
	pptr->store = avail;
	pptr->vhpnpages = hsize;
	pptr->vhpno = vpno; // should be 20 bits, vpno's don't have to be unique, operate on pd located at pdbr
	pptr->store = avail; // set when page table created
	pptr->pdbr = (unsigned long)*pd;
	restore(ps);
	return(pid);
}

// initializing vmemlist at 4096th page, not sure if this address is out of bounds, or maybe just if you try to dereference
// dreferencing may cause page fault and trigger pfintr.S
// needs to keep track of vpno's being used by each proc
// vmemlist should have at most 8 blocks, if 0 blocks no more virtual memory, each bs holds 256 pages, 2^11 pages should be max vm
WORD *init_vmemlist(struct mblock *vml, int vpno, int npages) {
	vml = (struct mblock *) vpno;
	pptr->vmemlist->mnext = 0;
	// len is maxaddr - first vpno
	//pptr->vmemlist->mlen = MAX_INT - vpno;
	pptr->vmemlist->mlen = npages;
}

// page directory consists of 1024 32 bit entries
// every process should use the 4 page tables created in initialize.c for the first 16 MB of memory (first 4096 pages)
// each process has 1 page directory, it occupies single page in memory, 4 KB
struct pd_t *create_page_dir(int bs_id, int vpno, int pid) {
	int i, avail;
	get_frm(&avail);
	fr_map_t *frm = &frm_tab[avail];
    frm->fr_status = FRM_MAPPED;
    frm->fr_pid = pid;
    frm->fr_refcnt = 1;
    frm->fr_type = FR_DIR;
    frm->fr_dirty = 0;
    frm->fr_vpno = vpno;
	unsigned long frm_addr = avail * NBPG;
  	struct pd_t *pd = (struct pd_t *)frm_addr;
	//struct pd_t *pd = (struct *pd_t)getmem(sizeof(struct pd_t) * 1024); // this address needs to be divisible by NBPG
	for(i = 0; i < 1024; i++) {
		pd->pd_write = 1;
		pd->pd_user = 0;
		pd->pd_pwt = 0;
		pd->pd_pcd = 0;
		pd->pd_acc = 0;
		pd->pd_mbz = 0;
		pd->pd_fmb = 0;
		pd->pd_global = 0;
		pd->pd_avail = 0;
		if(i == 0 || i == 1 || i == 2 || i == 3) {
			pd->pd_pres = 1;
			pd->pd_base = gpts[i];
		}
		else {
			pd->pd_pres = 0;
			pd->pd_base = NULL;
		}
		pd++;
	}
	pd = pd - 1024;
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

// mblock->mlen starts as difference between top address and bottom address in free frames (# of bytes difference)
/*
WORD *getvhp(int hsize) {
	struct	mblock	*p, *q, *leftover;
	unsigned int nbytes = hsize * NBPG;
	if (nbytes==0 || vmemlist.mnext== (struct mblock *) NULL) {
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
*/

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