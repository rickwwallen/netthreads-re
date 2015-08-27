TARGET=pipe
include ../bench.mk

$(TARGET): process.o iomem_send.o

cp:
	scp pipe.instr.mif pipe.instr.enc pipe.data.mif pipe.hcode ${SKY}$(CPFOLDER)
