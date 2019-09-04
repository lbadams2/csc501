/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
extern void syscallsummary_start();
extern void syscallsummary_stop();
extern void printsyscallsummary();
extern void print_arr_debug();

int main()
{
	kprintf("\n\nHello World, Xinu lives\n\n");
	syscallsummary_start();
	//print_arr_debug();
    getpid();
    syscallsummary_stop();
    printsyscallsummary();
	return 0;
}
