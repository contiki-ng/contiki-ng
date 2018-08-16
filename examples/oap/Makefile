BOOTLOADER = bootloader/bootloader.hex
GOLDEN_IMAGE = golden-image/golden-image.hex
FINAL_IMAGE = image.hex

all: $(FINAL_IMAGE)

.PHONY: clean distclean

clean:
	rm -f $(FINAL_IMAGE)
	
distclean: clean
	$(MAKE) -C $(dir $(BOOTLOADER)) clean
	$(MAKE) -C $(dir $(GOLDEN_IMAGE)) clean

FORCE:

$(BOOTLOADER): FORCE
	$(MAKE) -C $(dir $@) $(notdir $@)
	
$(GOLDEN_IMAGE): FORCE
	$(MAKE) -C $(dir $@) $(notdir $@)

$(FINAL_IMAGE): $(BOOTLOADER) $(GOLDEN_IMAGE) FORCE
	srec_cat $(BOOTLOADER) -intel -crop 0x00000000 0x00002000 0x0001FFA8 0x00020000 \
		$(GOLDEN_IMAGE) -intel -crop 0x00002000 0x0001FFA8 \
		-o $@ -intel
