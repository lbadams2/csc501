/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode
pfintr:
    pushl %ebp # pushl automatically decrements esp
    movl %esp, %ebp # save old sp
    outb %ebp, $pferrcode # error code in eax
    pushfl # push flags onto stack
    cli # clear interrupts
    pusha # stores general registers
    call pfint
    popa # restores general registers
    popfl # restore flags
    mov %ebp, %esp # this and below line are equivalent to leave, get old sp back
    popl %ebp
    sti # restore interrupts
    iret