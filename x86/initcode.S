.text
main:
        // exec /init
        movl    $1, %eax
        movq    $init, %rdi
        movq    $argv, %rsi
        int     $0x80
1:
        hlt
        jmp     1b

init:
        .asciz "/init"
.p2align 3
argv:
        .quad init
        .quad 0
