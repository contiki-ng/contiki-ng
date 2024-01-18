CONTIKI_PROJECT = simple-tz
CONTIKI = ../../../..

ifeq ($(TARGET),)
    -include Makefile.target
endif

S_TARGETBIN=secure/secure.$(TARGET).bin
NS_TARGETBIN=nonsecure/nonsecure.$(TARGET).bin

SECURE_SIZE_TOOL_DIR = $(CONTIKI)/tools/secure_size
SECURE_SIZE_BIN = $(SECURE_SIZE_TOOL_DIR)/secure_size.$(TARGET).$(BOARD)

all: $(CONTIKI_PROJECT).$(TARGET).bin

$(CONTIKI_PROJECT).$(TARGET).bin:$(S_TARGETBIN) $(NS_TARGETBIN) $(SECURE_SIZE_BIN)
	@cp $(S_TARGETBIN) $@
	@dd if=$(NS_TARGETBIN) of=$@ bs=1 seek=`$(SECURE_SIZE_BIN)`

$(S_TARGETBIN):
	$(MAKE) -C secure TOP=$(CONTIKI)

$(NS_TARGETBIN):
	$(MAKE) -C nonsecure TOP=$(CONTIKI)

%:
	$(MAKE) $@ -C secure TOP=$(CONTIKI)
	$(MAKE) $@ -C nonsecure TOP=$(CONTIKI)

.PHONY: all clean $(S_TARGETBIN) $(NS_TARGETBIN) cscope
clean:
	rm -f $(CONTIKI_PROJECT).$(TARGET).bin cscope*
	$(MAKE) $@ -C secure TOP=$(CONTIKI)
	$(MAKE) $@ -C nonsecure TOP=$(CONTIKI)

$(SECURE_SIZE_BIN): $(SECURE_SIZE_TOOL_DIR)/secure_size.c
	$(MAKE) -C $(SECURE_SIZE_TOOL_DIR) TARGET=$(TARGET) BOARD=$(BOARD)

cscope:
	cscope -Rbq -s $(CONTIKI)/arch/cpu/$(TARGET) -s $(CONTIKI)/arch/platform/$(TARGET)/$(BOARD) -s $(CONTIKI)/os
