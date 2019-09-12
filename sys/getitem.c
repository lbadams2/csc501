/* getitem.c - getfirst, getlast */

#include <conf.h>
#include <kernel.h>
#include <q.h>

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

// need to implement round robin if processes have same priority
int get_exp_proc(double rand_val, int head) {
	int cur = head;
	bool one_greater = false;
	while(q[cur].qkey < rand_val) {
		if(q[cur].qnext == EMPTY)
			break;
		cur = q[cur].qnext;
		one_greater = true;
	}
	if(one_greater)
		return( dequeue(q[cur].qprev );
	else
		return( dequeue(cur) );
}
