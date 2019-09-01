#include <conf.h>
#include <kernel.h>
#include <stdio.h>

extern char edata;
extern int etext;
extern WORD _end;

void printsegaddress() {
        kprintf("first address past program text (etext) 0x%08x with contents 0x%08x\n", &etext, etext);
        kprintf("first address past initialized data (edata) 0x%08x with contents 0x%08x\n", &edata, edata);
        kprintf("first address past uninitialized data (end/BSS) 0x%08x with contents 0x%08x\n", &_end, _end);

        char* tmp_etext = &etext - 1;
        char* tmp_edata = &edata - 1;
        char* tmp_end = &_end - 1;
        kprintf("Address preceding program text (etext) 0x%08x with contents 0x%08x\n", tmp_etext, *tmp_etext);
        kprintf("Address preceding initialized data (edata) 0x%08x with contents 0x%08x\n", tmp_edata, *tmp_edata);
        kprintf("Address preceding uninitialized data (end/BSS) 0x%08x with contents 0x%08x\n", tmp_end, *tmp_end);

        tmp_etext = &etext + 2;
        tmp_edata = &edata + 2;
        tmp_end = &_end + 2;
        kprintf("Address after program text (etext) 0x%08x with contents 0x%08x\n", tmp_etext, *tmp_etext);
        kprintf("Address after initialized data (edata) 0x%08x with contents 0x%08x\n", tmp_edata, *tmp_edata);
        kprintf("Address after uninitialized data (end/BSS) 0x%08x with contents 0x%08x\n", tmp_end, *tmp_end);
}