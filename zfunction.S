#include <icu.s>
                .text
.globl  zfunction
zfunction:
        pushl %ebp # pushl automatically decrements esp
        movl %esp, %ebp
        movl 8(%ebp), %eax
        and    $0xfffc01ff,%eax
        sar    $0x8,%eax
        mov %ebp, %esp # this and below line are equivalent to leave
        popl %ebp
        ret