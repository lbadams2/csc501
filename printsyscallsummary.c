#include <proc.h>
#include <stdio.h>
#include <lab0.h>

void initsysarr() {
    int i, j;
    scdataarrsize = 0;
    for (i = 0; i < NPROC; i++) {
        struct syscalldata arr[27];
        for(int j = 0; j < 27; j++) {
            scdataarr[i][j].name = "none";
            scdataarr[i][j].procid = currpid;
        }
        scdataarr = arr;
        scdataarr++;
        scdataarrsize++;
    }
    
}

void updatesysarr(char* name, unsigned long duration) {
    struct syscalldata* sc;
    int i, j;
    bool foundsc = false;
    bool foundproc = false;
    for(i = 0; i < NPROC; i++) {
            sc = scdataarr[i];
            if(sc->procid != currpid)
                continue;
            for(int j = 0; j < 27; j++) {
                if(sc[j].name == name) {
                        foundsc = true;
                        break;
                }
                else if(sc[j].name == "none")
                    break;
            }
    }
    if(foundsc) {
            sc[j].numcalls++;
            sc[j].durations = duration;
            sc[j].durations++;
    } else if(foundproc){
            sc[j].name = name;
            sc[j].numcalls = 1;
            sc[j].durations = duration;
            sc[j].durations++;
    } else {
        struct syscalldata arr[27];
        for(int j = 0; j < 27; j++) {
            if(j != 0) {
                scdataarr[i][j].name = "none";
                scdataarr[i][j].procid = currpid;
            } else {
                scdataarr[i][j].name = name;
                scdataarr[i][j].procid = currpid;
                sc[j].numcalls = 1;
                sc[j].durations = duration;
                sc[j].durations++;
            }
        }
        scdataarr = arr;
        scdataarr++;
        scdataarrsize++;
    }
}

void printsyscallsummary() {
    int i, j, thepid;
    struct syscalldata* sc;
    for(i = 0; i < scdataarrsize; i++) {
        sc = scdataarr[i];
        thepid = sc->procid;
        kprintf("Process [pid:%d]\n", thepid);
        for(int j = 0; j < 27; j++) {
            if(sc[j].name != "none") {
                int k;
                unsigned long sum = 0;
                unsigned long* durations = sc[j].durations;
                for(k = 0; k < sc[j].numcalls; k++)
                    sum += durations[k];
                double avg = sum / sc[j].numcalls;
                kprintf("\tSyscall: %s, count: %d, average execution time: %d (ms)\n", sc[j].name, avg)
            }
        }
    }
}

void syscallsummary_start() {
    initsysarr();
}

void syscallsummary_stop() {
}