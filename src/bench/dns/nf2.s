# 1 "../common/nf2.S"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "../common/nf2.S"
# 13 "../common/nf2.S"
# 1 "../common/num_cpu.h" 1
# 14 "../common/nf2.S" 2

.equ CONNADDR, (1<<26)


.equ _spdiff, 32 # power of 2, adds to stack diff in init_stack
.text
.align 2
.globl _start
.globl _t1
.globl _exit
.ent _start
_start:
.set noreorder

    nop # Must start with nop since 1st instr executed twice

    nop
    nop

    lui $28,%hi(_gp) # set the global data pointer (value in lsi.ld)
    addiu $28,$28,%lo(_gp)
    lui $29,%hi(_sp) # set the stack pointer (value in lsi.ld)
    addiu $29,$29,%lo(_sp)

    addiu $26,$0,0
    jal init_stack
    jal main
    nop
    j _exit

.end _start


.globl _t1
.ent _t1
_t1: # this is for ST
_t01:
 nop
 nop
 nop
 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-1*_spdiff)
 addiu $26,$0,1
 jal init_stack
 jal main
 j _exit
 nop
.end _t1
.globl _t2
.ent _t2
_t2: # this is for ST
_t02:
 nop
 nop
 nop
 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-2*_spdiff)
 addiu $26,$0,2
 jal init_stack
 jal main
 j _exit
 nop
.end _t2
.globl _t3
.ent _t3
_t3: # this is for ST
_t03:
 nop
 nop
 nop
 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-3*_spdiff)
 addiu $26,$0,3
 jal init_stack
 jal main
 j _exit
 nop
.end _t3
.globl _t4
.ent _t4
_t4: # this is for ST
_t04:
 nop
 nop



 nop

 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-4*_spdiff)
 addiu $26,$0,4
 jal init_stack
 jal main
 j _exit
 nop
.end _t4
.globl _t5
.ent _t5
_t5: # this is for ST
_t05:
 nop
 nop



 nop

 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-5*_spdiff)
 addiu $26,$0,5
 jal init_stack
 jal main
 j _exit
 nop
.end _t5
.globl _t6
.ent _t6
_t6: # this is for ST
_t06:
 nop
 nop



 nop

 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-6*_spdiff)
 addiu $26,$0,6
 jal init_stack
 jal main
 j _exit
 nop
.end _t6
.globl _t7
.ent _t7
_t7: # this is for ST
_t07:
 nop
 nop



 nop

 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-7*_spdiff)
 addiu $26,$0,7
 jal init_stack
 jal main
 j _exit
 nop
.end _t7
.globl _t8
.ent _t8
_t8: # this is for ST
_t08:
 nop
 nop
 j _t8 # nop
 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-8*_spdiff)
 addiu $26,$0,8
 jal init_stack
 jal main
 j _exit
 nop
.end _t8
.globl _t9
.ent _t9
_t9: # this is for ST
_t09:
 nop
 nop
 j _t9
 lui $28,%hi(_gp)
 lui $29,%hi(_sp)
 addiu $28,$28,%lo(_gp)
 addiu $29,$29,%lo(_sp-9*_spdiff)
 addiu $26,$0,9
 jal init_stack
 jal main
 j _exit
 nop
.end _t9



.ent _exit
_exit:
    lui $28, %hi(_gp)
    ori $28, $28, %lo(_gp)
    lui $2, 0xdead
    ori $2, $2, 0xdead
    sw $2, 0($28) # write 0xdeaddead overwriting results (who cares)
    lui $2, 0
    ori $16, $0, 0
    ori $17, $0, 0
    ori $18, $0, 0
    ori $19, $0, 0
    ori $20, $0, 0
    ori $21, $0, 0
    ori $22, $0, 0
    ori $23, $0, 0
    ori $30, $0, 0
    j _exit
    nop
    nop
    nop

    .set reorder
    .end _exit
