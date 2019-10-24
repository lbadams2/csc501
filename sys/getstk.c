/* getstk.c - getstk */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * getstk  --  allocate stack memory, returning address of topmost WORD
 *------------------------------------------------------------------------
 */
WORD *getstk(unsigned int nbytes)
{
	STATWORD ps;    
	struct	mblock	*p, *q;	/* q follows p along memlist		*/
	struct	mblock	*fits, *fitsq=NULL;
	WORD	len;

	disable(ps);
	if (nbytes == 0) {
		restore(ps);
		return( (WORD *)SYSERR );
	}
	nbytes = (unsigned int) roundew(nbytes);
	fits = (struct mblock *) NULL;
	q = &memlist;
	for (p = q->mnext ; p !=(struct mblock *) NULL ; q = p,p = p->mnext)
		if ( p->mlen >= nbytes) {
			fitsq = q;
			fits = p;
		}
	if (fits == (struct mblock *) NULL) {
		restore(ps);
		return( (WORD *)SYSERR );
	}
	if (nbytes == (len = fits->mlen) ) {
		fitsq->mnext = fits->mnext;
	} else {
		fits->mlen -= nbytes;
	}
	fits = (struct mblock *) ((WORD) fits + len - sizeof(WORD));
	*((WORD *) fits) = nbytes;
	restore(ps);
	// mem list ranges from HOLEEND to maxaddr
	// any memory not allocated through getstk or virtual mem will be in the kernel mem?
	return( (WORD *) fits);
}
