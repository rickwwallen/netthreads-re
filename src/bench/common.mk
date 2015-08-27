PKG_BASE ?= $(CURDIR)/../../..
BASE=$(realpath $(PKG_BASE))

NF_HOME ?= $(BASE)/compiler
NF_TOOLS ?= $(BASE)/compiler/lib

SPE_PREFIX=$(NF_HOME)/bin/mips-mips-elf
SPE_COMPILER=$(SPE_PREFIX)-gcc
SPE_AS=$(SPE_PREFIX)-as 
SPE_AR=$(SPE_PREFIX)-ar 
SPE_LD=$(SPE_PREFIX)-ld 
SPE_DIS=$(SPE_PREFIX)-objdump 
SPE_CPP=$(SPE_PREFIX)-cpp

EXTRA=-march=sp -mbias${BYPASS} -mmult${MULT} -mnodly${DLY} -Wall

NEW_ALLOC=../common/lib/uni_alloc.a
FUNC_LIB=$(NF_TOOLS)/libspe.a
FLOATING= $(NF_TOOLS)/libgcc.a
floating_present := $(wildcard $(FLOATING))
ifeq ($(strip $(floating_present)),) 
FLOATING= $(NF_TOOLS)/libgcc/soft-float/libgcc.a
endif

MATH= $(NF_TOOLS)/mylibm.a
EXTRA_LIB=$(FUNC_LIB) $(FUNC_LIB) $(NF_TOOLS)/mylibc.a

#ORIG_LIB = -L $(GCCLIB)/soft-float -lm -lc  -lgcc  $(FUNC_LIB)
ORIG_LIB = $(FUNC_LIB) $(NF_TOOLS)/mylibc.a  $(MATH) $(NF_TOOLS)/mylibc.a $(FLOATING) $(FUNC_LIB)
LDFLAGS= -T $(LSI) $(ORIG_LIB) --gc-sections

CFLAGS= -O3  -mgp32 -msoft-float -mlong32 $(EXTRA)  -fno-builtin  -DINTERNAL_NEWLIB -DDEFINE_MALLOC  -DREENTRANT_SYSCALLS_PROVIDED -ffunction-sections -fdata-sections $(UFLAGS) # -mno-rnames  -mips1

ASFLAGS= -O3 -march=sp
INCS = $(UDEFINCS)

#cancel this implicit rule, saves putting the command line option
#this causes the compilation of c files to go through assembly
#which is needed to get rid of shifts
%.o : %.c
%.o : %.C
%.s : %.S

#.c.o: 
#	$(SPE_COMPILER) $(CFLAGS) -c -o $*.o $<


%.h: %.H
	m4 ${MACROS} $*.H > $*.h

%.c: %.C
	m4 $(MACROS) $*.C > $*.c

# if you change this, don't forget to change scripts in tool/libgcc
%.o:%.s
	$(SPE_AS) $(ASFLAGS) -o $*.o $<



%.s:%.c
	$(SPE_COMPILER) $(CFLAGS) $(INCS) -S -o $*.s $<


#################  DEFAULTS ##############
# override these in the individual Makefiles if you wish
##########################################

DLY?=b
MULT?=o
START ?= nf2.o
LSI ?= ../common/nf2_linker.ld
UDEFINCS ?= -I. -I../common -I/usr/include -I$(BASE)/compiler/lib/gcc/mips-mips-elf/4.0.2/include/
VPATH?=../common

