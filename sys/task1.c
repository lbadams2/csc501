#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <lock.h>
#include <stdio.h>


void reader(char *msg, int lck)
{
    int     ret;

    kprintf ("  %s: to acquire lock\n", msg);
    lock (lck, READ, 20);
    kprintf ("  %s: acquired lock\n", msg);
    kprintf ("  %s: to release lock\n", msg);
    releaseall (1, lck);
}

void writer(char *msg, int lck)
{
    kprintf ("  %s: to acquire lock\n", msg);
    lock (lck, WRITE, 20);
    kprintf ("  %s: acquired lock, sleep 10s\n", msg);
    sleep (10);
    kprintf ("  %s: to release lock\n", msg);
    releaseall (1, lck);
}


void reader_sem(char *msg, int sem)
{
    int     ret;

    kprintf ("  %s: to acquire lock\n", msg);
    wait(sem);
    kprintf ("  %s: acquired lock\n", msg);
    kprintf ("  %s: to release lock\n", msg);
    signal(sem);
}

void writer_sem(char *msg, int sem)
{
    kprintf ("  %s: to acquire lock\n", msg);
    wait(sem);
    kprintf ("  %s: acquired lock, sleep 10s\n", msg);
    sleep (10);
    kprintf ("  %s: to release lock\n", msg);
    signal(sem);
}


void test_sem() {
    int sem;
    int rd1, rd2;
    int wr1;
    sem = screate(1);
    rd1 = create(reader_sem, 2000, 25, "reader", 2, "reader A", sem);
    rd2 = create(reader_sem, 2000, 30, "reader", 2, "reader B", sem);
    wr1 = create(writer_sem, 2000, 20, "writer", 2, "writer", sem);

    resume(wr1);
    sleep (1);

    resume(rd1);
    sleep (1);
    kprintf("wr1 prio %d\n", getprio(wr1));

    resume (rd2);
	sleep (1);
	kprintf("wr1 prio %d\n", getprio(wr1));
	
	kill (rd2);
	sleep (1);
	kprintf("wr1 prio %d\n", getprio(wr1));

	kill (rd1);
	sleep(1);
	kprintf("wr1 prio %d\n", getprio(wr1));
}


void test_lck() {
    int     lck;
    int     rd1, rd2;
    int     wr1;

    lck  = lcreate ();

    rd1 = create(reader, 2000, 25, "reader", 2, "reader A", lck);
    rd2 = create(reader, 2000, 30, "reader", 2, "reader B", lck);
    wr1 = create(writer, 2000, 20, "writer", 2, "writer", lck);

    resume(wr1);
    sleep (1);

    resume(rd1);
    sleep (1);
	kprintf("writer prio %d", getprio(wr1));

    resume (rd2);
	sleep (1);
	kprintf("writer prio %d", getprio(wr1));
	
	kill (rd2);
	sleep (1);
	kprintf("writer prio %d", getprio(wr1));

	kill (rd1);
	sleep(1);
	kprintf("writer prio %d", getprio(wr1));
}