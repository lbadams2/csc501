#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

extern int track_sys_calls;
/*
void initsysarr() {
    int i, j;
    scdataarrsize = 0;
    for (i = 0; i < NPROC; i++) {
        struct syscalldata arr[27];
        for(j = 0; j < 27; j++) {
            //scdataarr[i][j].name = "none";
            scdataarr[i][j].procid = currpid;
        }
        //*scdataarr = arr;
        //scdataarr++;
        scdataarrsize++;
    }
    
}
*/

void updatesysarr(char* name, unsigned long duration) {
    kprintf("in update sys arr function %s %d\n", name, duration);
    struct syscalldata* sc;
    int i = 0, j = 0, num_durations = 0, free_row = -1;
    int foundsc = 0;
    int foundproc = 0;
    for(i = 0; i < 50; i++) {
            sc = scdataarr[i];
            if(sc->procid != currpid) {
                if(sc->procid == -1 && free_row == -1)
                    free_row = i;
                continue;
            }
	        foundproc = 1;
            for(j = 0; j < 27; j++) {
                if(strcmp(sc[j].name, name) == 0) {
                        foundsc = 1;
                        break;
                }
                else if(strcmp(sc[j].name, "none") == 0) {
                    break;
                }
            }
    }
    if(foundsc == 1) {
            num_durations = sc[j].numcalls++;
            if(num_durations > 49)
                num_durations = 49;
            sc[j].durations[num_durations] = duration;
            kprintf("If: Added %s with duration %d, row %d col %d duration index %d\n", name, duration, i, j, num_durations);
            //sc[j].durations++;
    } else if(foundproc == 1) {
        strcpy(scdataarr[i][j].name, name);
        num_durations = sc[j].numcalls++;
        if(num_durations > 49)
            num_durations = 49;
        sc[j].durations[num_durations] = duration;
        kprintf("Else if: Added %s with duration %d, row %d col %d duration index %d\n", name, duration, i, j, num_durations);
    } else {
        //kprintf("didn't find sc or process, free row is %d, j is %d\n", free_row, j);
        if(free_row == -1)
            free_row = 49;
        strcpy(scdataarr[free_row][j].name, name);
        num_durations = scdataarr[free_row][j].numcalls++;
        scdataarr[free_row][j].durations[num_durations] = duration;
        //kprintf("name is %s duration is %d\n", scdataarr[free_row][j].name, scdataarr[free_row][j].durations[0]);
        for(j = 0; j < 27; j++) {
            scdataarr[free_row][j].procid = currpid;
        }
        kprintf("Else: Added %s with duration %d, row %d col %d duration index %d\n", name, duration, free_row, j, num_durations);
        //*scdataarr = arr;
        //scdataarr++;
        scdataarrsize++;
    }
}

void printsyscallsummary() {
    int i, j, thepid;
    struct syscalldata* sc;
    if(scdataarrsize  > 49)
        scdataarrsize = 49;
    kprintf("scdataarrsize is %d\n", scdataarrsize);
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
                kprintf("\tSyscall: %s, count: %d, average execution time: %d (ms)\n", sc[j].name, sc[j].numcalls, avg);
            }
        }
    }
}

void print_arr_debug() {
    kprintf("\nDEBUG printing array\n\n");
    int i, j;
    for (i = 0; i < 50; i++) {
        kprintf("\nProcess number %d\n", i);
        for(j = 0; j < 27; j++) {
            kprintf("Struct %d name: %s, proc id: %d\n", j, scdataarr[i][j].name, scdataarr[i][j].procid);
        }
    }
    kprintf("\n\nDEBUG done printing array\n\n");
}

void initsysarr() {
    int i, j;
    scdataarrsize = 0;
    kprintf("currpid is %d\n", currpid);
    kprintf("nproc is %d\n", NPROC);
    for (i = 0; i < 50; i++) {
        //syscalldata arr[27];
        for(j = 0; j < 27; j++) {
	        strcpy(scdataarr[i][j].name, "none");
            //scdataarr[i][j].name = "none";
            scdataarr[i][j].procid = -1;
            scdataarr[i][j].numcalls = 0;
        }
        //scdataarr[i] = arr;
        //scdataarr++;
        //scdataarrsize++;
    }          
}

void syscallsummary_start() {
    track_sys_calls = 1;
    initsysarr();
}

void syscallsummary_stop() {
    track_sys_calls = 0;
}

