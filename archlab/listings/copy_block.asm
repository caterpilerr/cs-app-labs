# Execution begins at address 0
    .pos 0
    irmovq stack, %rsp                     # Set up stack pointer
    call main                              # Execute main program
    halt                                   # Terminate program

    .align 8
# Source block
src:
    .quad 0x00a
    .quad 0x0b0
    .quad 0xc00
# Destination block
dest:
    .quad 0x111
    .quad 0x222
    .quad 0x33

main: 
    irmovq $3,%rdx
    irmovq dest,%rsi
    irmovq src,%rdi
    call copy_block                        # copy_block(src, dest, 3) 
    ret

# long copy_block(long *src, long *dest, long len) 
copy_block:
    irmovq $0,%rax
    andq %rdx,%rdx
    jle test
    loop: mrmovq (%rdi),%r10
    irmovq $8,%r11
    addq %r11,%rdi
    rmmovq %r10,(%rsi)
    addq %r11,%rsi
    xorq %r10,%rax
    irmovq $1,%r11
    subq %r11,%rdx
    andq %rdx,%rdx
    jg loop
    test: ret

# Stack starts here and grows to lower addresses
    .pos 0x200
stack:
