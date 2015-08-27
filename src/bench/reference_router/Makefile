TARGET=reference_router
include ../bench.mk

$(TARGET): process.o pktbuff.o memcpy.o hashtable.o memory.o  iomem_send.o

cp:
	scp  $(TARGET).instr.mif $(TARGET).instr.enc  $(TARGET).hcode  $(TARGET).data.mif nf19:pktgen/router
