#ifndef _KERNEL_H_
#define _KERNEL_H_


#define EXPDISTSCHED 1
#define LINUXSCHED 2

void setschedclass (int sched_class);
int getschedclass();
void init_epoch();

#endif