/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
		STATWORD ps;    
	struct	mblock	*p, *q;
	unsigned top;

	//if (size==0 || (unsigned)block>(unsigned)maxaddr
	//    || ((unsigned)block)<((unsigned) &end))
	//	return(SYSERR);
	size = (unsigned)roundmb(size);
	disable(ps);
	// go while p is less than block and p is not null, in which case q should be block and vmemlist only has 1 block
	for( p=vmemlist.mnext,q= &vmemlist;
	     p != (struct mblock *) NULL && p < block ;
	     q=p,p=p->mnext )
		;
	// top is top address in q, q follows p, if q is greater than block and q is not first block in list
	// or block is greater than p there is an error
	if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= &vmemlist) ||
	    (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
		restore(ps);
		return(SYSERR);
	}
	// if top of q is address of block, expand q (merge block and q if they border each other)
	if ( q!= &memlist && top == (unsigned)block )
			q->mlen += size;
	// insert block between q and p
	else {
		block->mlen = size;
		block->mnext = p;
		q->mnext = block;
		q = block; // set for below if statement
	}
	// merge p and q(now block, original q still exists) if they are right next to each other
	if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
		q->mlen += p->mlen;
		q->mnext = p->mnext;
	}
	restore(ps);
	return(OK);
}
