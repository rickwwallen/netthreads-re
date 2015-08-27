TARGET=rick_hello
include ../bench.mk

$(TARGET): rick_hello.o pktbuff.o iomem_send.o

cp:
	scp rick_hello.instr.mif rick_hello.instr.enc rick_hello.data.mif rick_hello.hcode ${SKY}$(CPFOLDER)
