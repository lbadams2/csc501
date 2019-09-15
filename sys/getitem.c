/* getitem.c - getfirst, getlast */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * getfirst  --	 remove and return the first process on a list
 *------------------------------------------------------------------------
 */
int getfirst(int head)
{
	int	proc;			/* first process on the list	*/

	if ((proc=q[head].qnext) < NPROC)
		return( dequeue(proc) );
	else
		return(EMPTY);
}



/*------------------------------------------------------------------------
 * getlast  --  remove and return the last process from a list
 *------------------------------------------------------------------------
 */
int getlast(int tail)
{
	int	proc;			/* last process on the list	*/

	if ((proc=q[tail].qprev) < NPROC)
		return( dequeue(proc) );
	else
		return(EMPTY);
}

int handle_null(int index) {
	struct pentry* pptr = &proctab[index];
	//strcmp(nptr->pname, "prnull") == 0 && 
	if(pptr == &proctab[NULLPROC])  {
		int p;
		if( (p=q[rdytail].qprev) == index)  // null proc is only proc
			return index;
		else // found other proc
			return p;
	} else 
		return index;
}

// need to implement round robin if processes have same priority
// q tail next is EMPTY and key is MAXINT
int get_exp_proc(double rand_val, int head) {
	kprintf("rand val is %d\n", rand_val);
	print_proctab();
	int next, prev;
	for(prev=head,next=q[head].qnext ;
	    next != EMPTY && q[next].qkey < rand_val ; prev=next,next=q[next].qnext); // from insertd

	int proc;
	if(next == EMPTY)
		proc = handle_null(prev);
	else
		proc = handle_null(next);
	return( dequeue(proc) );
}
