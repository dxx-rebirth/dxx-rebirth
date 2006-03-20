                .text


.globl _CallBack
_CallBack:
                pushl           %ebp
                movl            %esp, %ebp

                pusha
                pushl           %ds
                pushl           %es
                pushl           %fs
                pushl           %gs

                movl            %cs: _WSockDataSel, %eax
                movw            %ax, %ds
                movw            %ax, %es
                movw            %ax, %fs
                movw            %ax, %gs

                movl            (5*4)(%ebp), %eax

                movl            %esp, (1022*4)(%eax)
                movl            %ss, (1023*4)(%eax)

                lea             (1022*4)(%eax), %edx
                pushl           %ds
                pushl           %edx
                lss             %ss:(%esp), %esp

                pushl           (1*4)(%eax)
                call            (%eax)
                addl            $4, %esp

                lss             %ss:(%esp), %esp

                popl            %gs
                popl            %fs
                popl            %es
                popl            %ds
                popa

                popl            %ebp

                lret
