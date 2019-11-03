/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode, pfint
pfintr:
    pushl %ebp # pushl automatically decrements esp
    movl %esp, %ebp # save old sp
    outb 4(%ebp), $pferrcode # error code in eax
    pushfl # push flags onto stack
    cli # clear interrupts
    pusha # stores general registers
    pfint
    popa # restores general registers
    popfl # restore flags
    mov %ebp, %esp # this and below line are equivalent to leave, get old sp back
    popl %ebp
    sti # restore interrupts
    iret