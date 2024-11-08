.globl main
main:

# KEEP THE ABOVE TWO LINES and
# Put your disassembled program BELOW...

lui x3, 65536
addi x5, x0, 255
add x6, x5, x5
add x7, x6, x5
addi x28, x7, -254
sw x5, 0(x3)
sw x6, 4(x3)
sw x7, 8(x3)
sw x28, 12(x3)
lw x29, 0(x3)
lw x30, 4(x3)
lw x31, 8(x3)
lw x4, 12(x3)
addi x3, x3, 16
sw x5, 0(x3)
sb x6, 4(x3)
sh x7, 8(x3)
sw x28, 12(x3)
addi x3, x3, 16
lb x12, -4(x3)
lh x13, -8(x3)
lbu x14, -12(x3)
lhu x15, -16(x3)
addi x17, x0, 93
ecall


