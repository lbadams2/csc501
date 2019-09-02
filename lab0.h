#ifndef Lab0_h
#define Lab0_h

struct syscalldata {
        char* name;
        unsigned long* durations;
        int numcalls;
        int procid;
};

struct  syscalldata** scdataarr;
int scdataarrsize;

#endif