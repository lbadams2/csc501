#include <icu.s>
                .text
                .globl  zfunction
zfunction:
        pushl %ecx
        pushl (%ebp)
        call puts
        addl $4, %esp           /* remove current argument.  */
        addl $4, %ebp

        popl %ecx

        ret
