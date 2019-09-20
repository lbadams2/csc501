#ifndef _SCHED_H_
#define _SCHED_H_


#define EXPDISTSCHED 1
#define LINUXSCHED 2

void setschedclass (int sched_class);
int getschedclass();
void init_epoch();
int get_linux_proc();
void print_proctab();

extern int rr_test_ix;
#endif