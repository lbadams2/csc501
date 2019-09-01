#include <icu.s>
                .text
.globl  zfunction
zfunction:
        pushl %ebp # pushl automatically decrements esp
        #pushl %ecx
        movl %esp, %ebp
        #movl 8(%ebp), %ecx
        #movl (%ecx), %ecx
        movl 8(%ebp), %eax
        mov %ebp, %esp # this and below line are equivalent to leave
        popl %ebp
        ret