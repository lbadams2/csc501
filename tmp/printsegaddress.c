#include <conf.h>
#include <kernel.h>
#include <stdio.h>

extern char edata;
extern int etext;
extern WORD _end;

void printsegaddress() {
        kprintf("\nvoid printsegaddress()\n");
        kprintf("Current: etext[0x%08x]=0x%08x, edata[0x%08x]=0x%08x, ebss[0x%08x]=0x%08x\n", &etext, etext, &edata, edata, &_end, _end);
        
        int* tmp_etext = &etext - 1;
        char* tmp_edata = &edata - 1;
        int* tmp_end = &_end - 1;
        kprintf("Preceding: etext[0x%08x]=0x%08x, edata[0x%08x]=0x%08x, ebss[0x%08x]=0x%08x\n", tmp_etext, *tmp_etext, tmp_edata, *tmp_edata, tmp_end, *tmp_end);

        tmp_etext = &etext + 1;
        tmp_edata = &edata + 1;
        tmp_end = &_end + 1;
        kprintf("After: etext[0x%08x]=0x%08x, edata[0x%08x]=0x%08x, ebss[0x%08x]=0x%08x\n", tmp_etext, *tmp_etext, tmp_edata, *tmp_edata, tmp_end, *tmp_end);
}