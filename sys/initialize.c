/* initialize.c - nulluser, sizmem, sysinit */

#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <sleep.h>
#include <mem.h>
#include <tty.h>
#include <q.h>
#include <io.h>
#include <paging.h>

/*#define DETAIL */
#define HOLESIZE	(600)	
#define	HOLESTART	(640 * 1024)
#define	HOLEEND		((1024 + HOLESIZE) * 1024)  
/* Extra 600 for bootp loading, and monitor */

extern	int	main();	/* address of user's main prog	*/

extern	int	start();

LOCAL		sysinit();

/* Declarations of major kernel variables */
struct	pentry	proctab[NPROC]; /* process table			*/
int	nextproc;		/* next process slot to use in create	*/
struct	sentry	semaph[NSEM];	/* semaphore table			*/
int	nextsem;		/* next sempahore slot to use in screate*/
struct	qent	q[NQENT];	/* q table (see queue.c)		*/
int	nextqueue;		/* next slot in q structure to use	*/
char	*maxaddr;		/* max memory address (set by sizmem)	*/
struct	mblock	memlist;	/* list of free memory blocks		*/
#ifdef	Ntty
struct  tty     tty[Ntty];	/* SLU buffers and mode control		*/
#endif

/* active system status */
int	numproc;		/* number of live user processes	*/
int	currpid;		/* id of currently running process	*/
int	reboot = 0;		/* non-zero after first boot		*/

int	rdyhead,rdytail;	/* head/tail of ready list (q indicies)	*/
char 	vers[80];
int	console_dev;		/* the console device			*/

/*  added for the demand paging */
int page_replace_policy = SC;

/************************************************************************/
/***				NOTE:				      ***/
/***								      ***/
/***   This is where the system begins after the C environment has    ***/
/***   been established.  Interrupts are initially DISABLED, and      ***/
/***   must eventually be enabled explicitly.  This routine turns     ***/
/***   itself into the null process after initialization.  Because    ***/
/***   the null process must always remain ready to run, it cannot    ***/
/***   execute code that might cause it to be suspended, wait for a   ***/
/***   semaphore, or put to sleep, or exit.  In particular, it must   ***/
/***   not do I/O unless it uses kprintf for polled output.           ***/
/***								      ***/
/************************************************************************/

/*------------------------------------------------------------------------
 *  nulluser  -- initialize system and become the null process (id==0)
 *------------------------------------------------------------------------
 */
nulluser()				/* babysit CPU when no one is home */
{
        int userpid;

	console_dev = SERIAL0;		/* set console to COM0 */

	initevec();

	kprintf("system running up!\n");
	sysinit();

	enable();		/* enable interrupts */

	sprintf(vers, "PC Xinu %s", VERSION);
	kprintf("\n\n%s\n", vers);
	if (reboot++ < 1)
		kprintf("\n");
	else
		kprintf("   (reboot %d)\n", reboot);


	kprintf("%d bytes real mem\n",
		(unsigned long) maxaddr+1);
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) 0);
	kprintf(" to %d\n", (unsigned long) (maxaddr) );
#endif	

	kprintf("%d bytes Xinu code\n",
		(unsigned long) ((unsigned long) &end - (unsigned long) start));
#ifdef DETAIL	
	kprintf("    %d", (unsigned long) start);
	kprintf(" to %d\n", (unsigned long) &end );
#endif

#ifdef DETAIL	
	kprintf("%d bytes user stack/heap space\n",
		(unsigned long) ((unsigned long) maxaddr - (unsigned long) &end));
	kprintf("    %d", (unsigned long) &end);
	kprintf(" to %d\n", (unsigned long) maxaddr);
#endif	
	
	kprintf("clock %sabled\n", clkruns == 1?"en":"dis");


	/* create a process to execute the user's main program */
	userpid = create(main,INITSTK,INITPRIO,INITNAME,INITARGS);
	resume(userpid);

	while (TRUE)
		/* empty */;
}

// should null proc get pages? what data should be put in the page?
struct pd_t *null_page_dir() {
	//struct pd_t *null_pd = (struct pd_t *)getmem(sizeof(struct pd_t) * 4); // this should be in free frames, addr divisible by NBPG
	int i, avail;
	get_frm(&avail);
	fr_map_t *frm = &frm_tab[avail];
    frm->fr_status = FRM_MAPPED;
    frm->fr_pid = pid;
    frm->fr_refcnt = 1;
    frm->fr_type = FR_DIR;
    frm->fr_dirty = 0;
    frm->fr_vpno = 0; // pd's and pt's aren't paged
	unsigned long frm_addr = avail * NBPG;
  	struct pd_t *null_pd = (struct pd_t *)frm_addr;
	for(i = 0; i < 4; i++) {
		null_pd->pd_pres = 1;
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
		null_pd->pd_base = gpts[i];
		null_pd++;
	}
	return null_pd - 4;
}

/*
void init_vmem_list() {
	struct	mblock	*mptr;
	void *vmem_start = 2048 * NBPG;
	void *vmem_end = 4095 * NBPG;
	vmemlist.mnext = mptr = (struct mblock *) roundmb(vmem_start);
	mptr = (struct mblock *) vmem_start;
	mptr->mnext = 0;
	mptr->mlen = (int) truncew((unsigned)vmem_end - vmem_start);
}


void init_vmem_list() {
	struct mblock *mptr;
	void *start_vmem = 4096 * NBPG;
	vmemlist.mnext = mptr = (struct mptr *) roundmb(start_vmem);
	 = (struct mblock *)start_vmem;
	mptr->mnext = 0;
	mptr->mlen = 0;
}
*/

void init_paging() {
	// first free address above kernel, 2^22 = 0x10000000000000000000000 (page 1024 = 1024*4096 = 2^22)
	// need 4 global page tables to map pages 0-4095, each has 1024 entries, 4096 pages*4096 page_size = 16 MB of memory mapped
	// starting address of each page table/directory must be divisible by NBPG. Each page table maps 4 MB of memory = 2^22
	// page is 4 KB, each page table contains 1024 PTEs
	int i, j;
	// for these global page tables the pt base corresponds to the address in physical memory
	// gpt is 4 bytes, creating 4096 gpts so gpts are 16 KB (4 pages)
	*gpts[4];
	for(i = 0; i < 4; i++) {		
		//struct pt_t *gpt = (struct pt_t *)getmem(sizeof(struct pt_t) * 1024); // this needs to be at addr divisible by NBPG
		int i, avail;
		get_frm(&avail);
		fr_map_t *frm = &frm_tab[avail];
		frm->fr_status = FRM_MAPPED;
		frm->fr_pid = pid;
		frm->fr_refcnt = 1;
		frm->fr_type = FR_TBL;
		frm->fr_dirty = 0;
		int vpno = i << 10; // offset into page dir, offset into page table is 0
		frm->fr_vpno = vpno;
		unsigned long frm_addr = avail * NBPG;
		struct pd_t *pd = (struct pd_t *)frm_addr;
		*gpts[i] = gpt;
		for(j = 0; j < 1024; j++) {
			gpt->pt_pres = 1;
			gpt->pt_write = 1; // this should only be write for 1024 - 4095
			gpt->pt_user = 0;
			gpt->pt_pwt = 0;
			gpt->pt_pcd = 0;
			gpt->pt_acc = 0;
			gpt->pt_dirty = 0;
			gpt->pt_mbz = 0;
			gpt->pt_global = 0;
			gpt->pt_avail = 0;
			gpt->pt_base = (i * NBPG) + j; // physical address of frame
			gpt++;
		}
	}
	// gpt is incremented 4096 times, adds 16 KB to initial free_frame addr 2^22 = 0x10000000100000000000000
		 
	pd_t *pd = null_page_dir();
	//create_inverted_pt();
	init_frm();
	init_bsm();
	// PDBR is cr3
	unsigned long null_pd_addr = (unsigned long)pd;
	write_cr3(null_pd_addr);
	void pfintr();
	// need to research first param more
	set_evec(40, (u_long)pfintr);
	// need to enable paging (PG flag in CRO register is set)
}

/*------------------------------------------------------------------------
 *  sysinit  --  initialize all Xinu data structeres and devices
 *------------------------------------------------------------------------
 */
LOCAL
sysinit()
{
	static	long	currsp;
	int	i,j;
	struct	pentry	*pptr;
	struct	sentry	*sptr;
	struct	mblock	*mptr;
	SYSCALL pfintr();

	

	numproc = 0;			/* initialize system variables */
	nextproc = NPROC-1;
	nextsem = NSEM-1;
	nextqueue = NPROC;		/* q[0..NPROC-1] are processes */

	/* initialize free memory list */
	/* PC version has to pre-allocate 640K-1024K "hole" */
	if (maxaddr+1 > HOLESTART) {
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = (struct mblock *)HOLEEND;
		mptr->mlen = (int) truncew(((unsigned) HOLESTART -
	     		 (unsigned)&end));
        mptr->mlen -= 4;

		mptr = (struct mblock *) HOLEEND;
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - HOLEEND -
	      		NULLSTK);
/*
		mptr->mlen = (int) truncew((unsigned)maxaddr - (4096 - 1024 ) *  4096 - HOLEEND - NULLSTK);
*/
	} else {
		/* initialize free memory list */
		memlist.mnext = mptr = (struct mblock *) roundmb(&end);
		mptr->mnext = 0;
		mptr->mlen = (int) truncew((unsigned)maxaddr - (int)&end -
			NULLSTK);
	}
	

	for (i=0 ; i<NPROC ; i++)	/* initialize process table */
		proctab[i].pstate = PRFREE;


#ifdef	MEMMARK
	_mkinit();			/* initialize memory marking */
#endif

#ifdef	RTCLOCK
	clkinit();			/* initialize r.t.clock	*/
#endif

	mon_init();     /* init monitor */

#ifdef NDEVS
	for (i=0 ; i<NDEVS ; i++ ) {	    
	    init_dev(i);
	}
#endif

	pptr = &proctab[NULLPROC];	/* initialize null process entry */
	pptr->pstate = PRCURR;
	for (j=0; j<7; j++)
		pptr->pname[j] = "prnull"[j];
	// should the page for null proc include these addresses, if null proc has a page?
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK;
	pptr->pbase = (WORD) maxaddr - 3;
/*
	pptr->plimit = (WORD)(maxaddr + 1) - NULLSTK - (4096 - 1024 )*4096;
	pptr->pbase = (WORD) maxaddr - 3 - (4096-1024)*4096;
*/
	pptr->pesp = pptr->pbase-4;	/* for stkchk; rewritten before used */
	*( (int *)pptr->pbase ) = MAGIC;
	pptr->paddr = (WORD) nulluser;
	pptr->pargs = 0;
	pptr->pprio = 0;
	currpid = NULLPROC;

	for (i=0 ; i<NSEM ; i++) {	/* initialize semaphores */
		(sptr = &semaph[i])->sstate = SFREE;
		sptr->sqtail = 1 + (sptr->sqhead = newqueue());
	}

	rdytail = 1 + (rdyhead=newqueue());/* initialize ready list */


	return(OK);
}

stop(s)
char	*s;
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* empty */;
}

delay(n)
int	n;
{
	DELAY(n);
}


#define	NBPG	4096

/*------------------------------------------------------------------------
 * sizmem - return memory size (in pages)
 *------------------------------------------------------------------------
 */
long sizmem()
{
	unsigned char	*ptr, *start, stmp, tmp;
	int		npages;

	/* at least now its hacked to return
	   the right value for the Xinu lab backends (16 MB) */

	return 2048; 

	start = ptr = 0;
	npages = 0;
	stmp = *start;
	while (1) {
		tmp = *ptr;
		*ptr = 0xA5;
		if (*ptr != 0xA5)
			break;
		*ptr = tmp;
		++npages;
		ptr += NBPG;
		if ((int)ptr == HOLESTART) {	/* skip I/O pages */
			npages += (1024-640)/4;
			ptr = (unsigned char *)HOLEEND;
		}
	}
	return npages;
}
