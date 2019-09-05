#ifndef Lab0_h
#define Lab0_h

typedef struct syscalldata {
        char name[20];
        unsigned long durations[50];
        int numcalls;
        int procid;
} syscalldata;

extern syscalldata scdataarr[][27];
int scdataarrsize;

extern void syscallsummary_start();
extern void syscallsummary_stop();
extern void printsyscallsummary();
extern void print_arr_debug();
extern long zfunction(long param);
extern void printsegaddress();
extern void printtos();
extern void printprocstks(int priority);

#endif
