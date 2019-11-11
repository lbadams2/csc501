/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>


void proc1_test1(char *msg, int lck) {
	char *addr;
	int i;

	get_bs(TEST1_BS, 100);

	if (xmmap(PROC1_VPNO, TEST1_BS, 100) == SYSERR) {
		kprintf("xmmap call failed\n");
		sleep(3);
		return;
	}

	addr = (char*) PROC1_VADDR;
	for (i = 0; i < 26; i++) {
		*(addr + i * NBPG) = 'A' + i;
	}

        /*
        unsigned long frame_addr;
        char *val;
        for(i = 2304; i < 2330; i++) {
                frame_addr = i * NBPG;
                val = (char *)frame_addr;
                *val = 'A' + i - 2304;
        }*/

	sleep(6);

	for (i = 0; i < 26; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}
        /*
        char *test_addr = 0;
        char *j;
        for(j = test_addr; j < 4096 * NBPG; j++) {
                if(*j == '@')
                        kprintf("0x%08x: %c\n", j, *(j));
        }
        
        for(j = test_addr - 30; j < test_addr; j++) {
                kprintf("0x%08x: %c\n", j, *(j));
        }
        for(j = test_addr; j < test_addr + 30; j++) {
                kprintf("0x%08x: %c\n", j, *(j));
        }*/
        //char *test_addr = 0;
        //for(i = 2304; i < 2330; i++) {
        //      kprintf("0x%08x: %c\n", i * NBPG, *(test_addr + i * NBPG));
        //}

	//xmunmap(PROC1_VPNO);
	return;
}


void proc1_test3(char *msg, int lck) {

	char *addr;
	int i;

	addr = (char*) 0x0;

	for (i = 0; i < 1024; i++) {
		*(addr + i * NBPG) = 'B';
	}

	for (i = 0; i < 1024; i++) {
		kprintf("0x%08x: %c\n", addr + i * NBPG, *(addr + i * NBPG));
	}

	return;
}

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
int main()
{
	kprintf("\n\nHello World, Xinu@QEMU lives\n\n");
        
        int pid1;
	int pid2;

        
	kprintf("\n1: shared memory\n");
	pid1 = create(proc1_test1, 2000, 20, "proc1_test1", 0, NULL);
	resume(pid1);
        sleep(10);

        /*
        kprintf("\n3: Frame test\n");
	pid1 = create(proc1_test3, 2000, 20, "proc1_test3", 0, NULL);
	resume(pid1);
	sleep(3);       
        */
}
