CONTIKI_DIR := $(abspath $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))/../..)

EXAMPLESDIR=$(HOME)/out-of-tree-tests

EXAMPLES = \
hello-world/native \
hello-world/zoul \
hello-world/openmote \
hello-world/cc26x0-cc13x0 \
hello-world/simplelink:BOARD=launchpad/cc26x2r1 \
hello-world/simplelink:BOARD=sensortag/cc2650 \

# Construct DIR_TARGET based on whether EXAMPLESDIR exists.
# Put a common dummy string first since targets cannot start with "-".
DIR_TARGET = examplesdir-$(wildcard $(EXAMPLESDIR))
DIR_PRESENT = examplesdir-$(EXAMPLESDIR)
DIR_ABSENT = examplesdir-

# Copy examples/hello-world to $(EXAMPLESDIR), and adjust Makefile.
preparation: | $(DIR_TARGET)
	@cp -a $(CONTIKI_DIR)/examples/hello-world $(EXAMPLESDIR)
	@perl -pi -e "s|^CONTIKI =.*$$|CONTIKI = $(CONTIKI_DIR)|g" $(EXAMPLESDIR)/hello-world/Makefile
	$(MAKE) all

$(DIR_ABSENT):
	@mkdir -p $(EXAMPLESDIR)

$(DIR_PRESENT):
	@echo "Remove $(EXAMPLESDIR) before running this test.\n"
	@false

include ../Makefile.compile-test
