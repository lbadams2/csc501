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
	unsigned size; // in pages
{
		STATWORD ps;    
	struct	mblock	*p, *q;
	unsigned top;
	int pid = getpid();
	struct mblock *vmemlist = &proctab[pid]->vmemlist;
	struct mblock *old_block = block;

	//if (size==0 || (unsigned)block>(unsigned)maxaddr
	//    || ((unsigned)block)<((unsigned) &end))
	//	return(SYSERR);
	size = (unsigned)roundmb(size);
	disable(ps);
	// go while p is less than block and p is not null, in which case q should be block and vmemlist only has 1 block
	for( p=vmemlist->mnext,q= &vmemlist;
	     p != (struct mblock *) NULL && p < block ;
	     q=p,p=p->mnext )
		;
	// top is top address in q, q follows p, if q is greater than block and q is not first block in list
	// or block is greater than p there is an error
	if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= vmemlist) ||
	    (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
		restore(ps);
		return(SYSERR);
	}
	// if top of q is address of block, expand q (merge block and q if they border each other)
	if ( q!= vmemlist && top == (unsigned)block )
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
	free_bs_blocks(old_block, size, pid);
	restore(ps);
	return(OK);
}

// current implementation doesn't handle crossing bs boundaries or heaps larger than a single backing store
void free_bs_blocks(struct mblock* vmemblock, unsigned size, int pid) {
	struct	mblock	*p, *q;
	unsigned top;
	unsigned long bs_addr = (unsigned long)vmemblock;
	bs_addr = bs_addr >> 12;
	bs_addr = bs_addr - 4096;
	int bs_id = bs_addr / 256;
	int offset = bs_addr % 256;
	bs_map_t *bs = &bsm_tab[bs_id];
	struct mblock *block = bs + offset;
	struct mblock *vmemlist = bs->free_list;


	for( p=vmemlist->mnext,q= vmemlist;
	     p != (struct mblock *) NULL && p < block ;
	     q=p,p=p->mnext )
		;
	// top is top address in q, q follows p, if q is greater than block and q is not first block in list
	// or block is greater than p there is an error
	if (((top=q->mlen+(unsigned)q)>(unsigned)block && q!= vmemlist) ||
	    (p!=NULL && (size+(unsigned)block) > (unsigned)p )) {
		restore(ps);
		return(SYSERR);
	}
	// if top of q is address of block, expand q (merge block and q if they border each other)
	if ( q!= vmemlist && top == (unsigned)block )
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
}
