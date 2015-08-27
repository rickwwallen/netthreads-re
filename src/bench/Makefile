SUBDIRS := bounce nat perf ping pipe printf udhcp classifier pktgen bugs drifttest

# These targets work on all the Makefiles in the SUBDIRS
SUBTARGETS := clean embed_clean mif rif embed redo

.PHONY: subdirs $(SUBDIRS) $(SUBTARGETS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

$(SUBTARGETS):
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done

NAMES=nat classifier udhcp ping

ex:
	for i in $(NAMES); do make -C "$${i}"; done


GETNUM=`awk '(NF==3) && ($$2 == "NUM_CPU"){print $$3}' common/num_cpu.h`
EXFOLDER=~/tmp/ex/exe$(GETNUM)
CPFOLDER=/exe$(GETNUM)

test:
	echo $(EXFOLDER)
	setenv EXFOLDER $(EXFOLDER)

CPNAMES=pipe  udhcp nat classifier pubsub intruder
CPNAMES= nat pipe udhcp classifier intruder reintruder
CPNAMES=   classifier nat udhcp reintruder
#udhcp classifier reintruder

excp:
	mkdir -p $(EXFOLDER)
	for i in $(CPNAMES); do cp "$${i}/$${i}" $(EXFOLDER); done
	for i in $(CPNAMES); do cp "$${i}/$${i}.haz" $(EXFOLDER); done
	for i in $(CPNAMES); do cp "$${i}/$${i}.map" $(EXFOLDER); done
#	scp  ~/tmp/ex/* npire@fang:ex

cp:
	for i in $(CPNAMES); do make -C "$${i}" CPFOLDER=$(CPFOLDER) cp ; done

MYCSTR=SPLIT
check:
	-for i in $(CPNAMES); do grep IQM $${i}/*.h; done
	-for i in $(CPNAMES); do grep $(MYCSTR) $${i}/*.h; done
	-for i in $(CPNAMES); do ls -l  $${i}/$${i}; done

edit:
	emacs -nw udhcp/dhcpd.h nat/myclassifier.h classifier/myclassifier.h reintruder/sub.h
