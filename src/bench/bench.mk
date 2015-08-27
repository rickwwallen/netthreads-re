# There are 3 possible contexts:
# nf: program will be cross-compiled for the netfpga.
# sim: program will be cross-compiled for the netfpga, but
#      will also support simprintf function which is useful
#      in the simulator.
# sw: program is built as a normal binary using the 
#     default tool chain. 

# default to the netfpga context
CONTEXT ?= nf

ifeq ($(CONTEXT),nf)
$(info ******* Cross-compiling for the netfpga. *******)
EXTRA_DEFINES+= -DCONTEXT=nf -DCONTEXT_NF
else 
ifeq ($(CONTEXT),sim)
$(info ******* Cross-compiling for the simulator. *******)
EXTRA_DEFINES+= -DCONTEXT=sim -DCONTEXT_SIM
else 
ifeq ($(CONTEXT),sw)
$(info ******* Compiling a normal binary. *******)
# this is with raw sockets
#EXTRA_DEFINES+= -DCONTEXT=sw -DCONTEXT_SW -DDEBUG -DSW_NETWORK
# this is with a packet trace
EXTRA_DEFINES+= -DCONTEXT=sw -DCONTEXT_SW -DDEBUG 
else
$(error Unknown CONTEXT="$(CONTEXT)". Must be one of "nf", "sim" or "sw")
endif
endif
endif

# Pass the NUM_CPU variable to the preprocessor if defined
ifneq ($(origin NUM_CPU), undefined)
EXTRA_DEFINES+=-DNUM_CPU=$(NUM_CPU)
endif

ifeq ($(CONTEXT),sw)

VPATH=../common
#ULIBS+=/usr/lib/libpcap.so.0.8 -lm
ULIBS+=/usr/lib/libpcap.so.1.4.0 -lm
#ULIBS+=/usr/lib64/libpcap.so.1.4.0 -lm
CFLAGS+=-Wall  $(EXTRA_DEFINES)
ADD_DEPS=sw_pkt_io.o sw_port.o sw_sniffer.o 
ADDITIONAL= ../common/lib/uni_alloc86.a
TARGETS ?= $(TARGET)

$(TARGETS):  $(ADD_DEPS)
	g++  -o $@ -g -Wall -lm  $^ $(ADDITIONAL) $(ULIBS)


.c.o:
	g++ $(CFLAGS) -c $< -o $@ -I. -I../common -g  $(UFLAGS)

.cc.o:
	g++ $(CFLAGS) -c $< -o $@ -I. -I../common -g  


clean: 
	rm -f *.o $(TARGETS)

else

include ../common.mk
CFLAGS+=$(EXTRA_DEFINES)

include ../footer.mk

endif

# Add some additional dependencies that all bench
# programs should include.
$(TARGETS): support.o support_no_inline.o memcpy.o

