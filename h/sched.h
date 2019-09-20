#ifndef _SCHED_H_
#define _SCHED_H_


#define EXPDISTSCHED 1
#define LINUXSCHED 2

void setschedclass (int sched_class);
int getschedclass();
void init_epoch();
int get_linux_proc();
void print_proctab();
void rr_enqueue(int proc);
int rr_dequeue();
int rr_isempty();
int rr_contains(int proc);
void init_rrq();


extern int rr_test_ix;
extern int rrq_front;
extern int rrq_back;
extern int rrq_size;

extern int rrq[];
#endif