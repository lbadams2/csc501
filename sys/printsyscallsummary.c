#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

void initsysarr() {
    int i, j;
    scdataarrsize = 0;
    for (i = 0; i < NPROC; i++) {
        struct syscalldata arr[27];
        for(j = 0; j < 27; j++) {
            scdataarr[i][j].name = "none";
            scdataarr[i][j].procid = currpid;
        }
        *scdataarr = arr;
        scdataarr++;
        scdataarrsize++;
    }
    
}

void updatesysarr(char* name, unsigned long duration) {
    struct syscalldata* sc;
    int i = 0, j = 0;
    int foundsc = 0;
    int foundproc = 0;
    for(i = 0; i < NPROC; i++) {
            sc = scdataarr[i];
            if(sc->procid != currpid)
                continue;
	        foundproc = 1;
            for(j = 0; j < 27; j++) {
                if(sc[j].name == name) {
                        foundsc = 1;
                        break;
                }
                else if(strcmp(sc[j].name, "none") == 0)
                    break;
            }
    }
    if(foundsc == 1) {
            sc[j].numcalls++;
            *(sc[j].durations) = duration;
            sc[j].durations++;
    } else if(foundproc == 1){
            sc[j].name = name;
            sc[j].numcalls = 1;
            *(sc[j].durations) = duration;
            sc[j].durations++;
    } else {
        struct syscalldata arr[27];
        for(j = 0; j < 27; j++) {
            if(j != 0) {
                scdataarr[i][j].name = "none";
                scdataarr[i][j].procid = currpid;
            } else {
                scdataarr[i][j].name = name;
                scdataarr[i][j].procid = currpid;
                sc[j].numcalls = 1;
                *(sc[j].durations) = duration;
                sc[j].durations++;
            }
        }
        *scdataarr = arr;
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
        for(j = 0; j < 27; j++) {
            if(strcmp(sc[j].name, "none") != 0) {
                int k;
                unsigned long sum = 0;
                unsigned long* durations = sc[j].durations;
                for(k = 0; k < sc[j].numcalls; k++)
                    sum += durations[k];
                double avg = sum / sc[j].numcalls;
                kprintf("\tSyscall: %s, count: %d, average execution time: %d (ms)\n", sc[j].name, avg);
            }
        }
    }
}

void print_arr_debug() {
    kprintf("\nDEBUG printing array\n\n");
    int i, j;
    for (i = 0; i < NPROC; i++) {
        kprintf("\nProcess number %d\n", i);
        for(j = 0; j < 27; j++) {
            kprintf("Struct %d name: %s, proc id: %s\n", j, scdataarr[i][j].name, scdataarr[i][j].procid);
        }
    }
    kprintf("\n\nDEBUG done printing array\n\n");
}

void syscallsummary_start() {
    initsysarr();
}

void syscallsummary_stop() {
}

