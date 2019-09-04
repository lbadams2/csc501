#ifndef Lab0_h
#define Lab0_h

typedef struct syscalldata {
        char name[20];
        unsigned long durations[100];
        int numcalls;
        int procid;
} syscalldata;

extern syscalldata scdataarr[][27];
int scdataarrsize;

#endif
