#/* $begin ncopy-ys */
##################################################################
# ncopy.ys - Copy a src block of len words to dst.
# Return the number of positive words (>0) contained in src.
#
# Include your name and ID here.
#
# Describe how and why you modified the baseline code.
#
##################################################################
# Do not modify this portion
# Function prologue.
# %rdi = src, %rsi = dst, %rdx = len
ncopy:

##################################################################
# You can modify this portion
	# Loop header
	xorq %rax, %rax
	iaddq $-5, %rdx
	jl EndUn	

LoopUn:	
	mrmovq (%rdi), %r10
	mrmovq 8(%rdi), %r11
	mrmovq 16(%rdi), %r12
	mrmovq 24(%rdi), %r13
	mrmovq 32(%rdi), %r9
	rmmovq %r10, (%rsi)
	rmmovq %r11, 8(%rsi)
	rmmovq %r12, 16(%rsi)
	rmmovq %r13, 24(%rsi)
	rmmovq %r9, 32(%rsi)

	andq %r10, %r10
	jle First		
	iaddq $1, %rax
First:
	andq %r11, %r11
	jle Second		
	iaddq $1, %rax
Second:
	andq %r12, %r12
	jle Third		
	iaddq $1, %rax
Third:	
	andq %r13, %r13
	jle Fourth		
	iaddq $1, %rax
Fourth:	
	andq %r9, %r9
	jle Fifth
	iaddq $1, %rax
Fifth:

	iaddq $40, %rdi		# src++
	iaddq $40, %rsi		# dst++
	iaddq $-5, %rdx		# len--
	jge LoopUn

EndUn:
	iaddq $5, %rdx
	je Done
	iaddq $-1, %rdx
	je One
	iaddq $-1, %rdx
	je Two 
	iaddq $-1, %rdx
	je Three 

	mrmovq (%rdi), %r10
	mrmovq 8(%rdi), %r11
	mrmovq 16(%rdi), %r12
	mrmovq 24(%rdi), %r13
	rmmovq %r10, (%rsi)
	rmmovq %r11, 8(%rsi)
	rmmovq %r12, 16(%rsi)
	rmmovq %r13, 24(%rsi)

	andq %r10, %r10
	jle F1 
	iaddq $1, %rax
F1:
	andq %r11, %r11
	jle F2 
	iaddq $1, %rax
F2:
	andq %r12, %r12
	jle F3 
	iaddq $1, %rax
F3:
	andq %r13, %r13
	jle Done 
	iaddq $1, %rax
	jmp Done

Three:
	mrmovq (%rdi), %r10
	mrmovq 8(%rdi), %r11
	mrmovq 16(%rdi), %r12
	rmmovq %r10, (%rsi)
	rmmovq %r11, 8(%rsi)
	rmmovq %r12, 16(%rsi)

	andq %r10, %r10
	jle Th1 
	iaddq $1, %rax
Th1:
	andq %r11, %r11
	jle Th2 
	iaddq $1, %rax
Th2:
	andq %r12, %r12
	jle Done
	iaddq $1, %rax
	jmp Done
Two:
	mrmovq (%rdi), %r10
	mrmovq 8(%rdi), %r11
	rmmovq %r10, (%rsi)
	rmmovq %r11, 8(%rsi)
	andq %r10, %r10
	jle T1 
	iaddq $1, %rax
T1:
	andq %r11, %r11
	jle Done
	iaddq $1, %rax
	jmp Done
One:
	mrmovq (%rdi), %r10
	rmmovq %r10, (%rsi)
	andq %r10, %r10
	jle Done
	iaddq $1, %rax
##################################################################
# Do not modify the following section of code
# Function epilogue.
Done:
	ret
##################################################################
# Keep the following label at the end of your function
End:
#/* $end ncopy-ys */

