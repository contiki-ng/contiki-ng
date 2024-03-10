all:
	$(MAKE) -C secure all
	$(MAKE) -C nonsecure all
%:
	$(MAKE) -C secure $@
	$(MAKE) -C nonsecure $@
.PHONY: all
