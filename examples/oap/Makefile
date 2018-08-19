BOOTLOADER = bootloader/bootloader.hex
GOLDEN_IMAGE = golden-image/golden-image.hex
METADATA = metadata.hex
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

$(METADATA): $(GOLDEN_IMAGE)
	python ../../tools/oap/generate-metadata.py -o $@ -O 0x1ff98 $<

$(FINAL_IMAGE): $(BOOTLOADER) $(GOLDEN_IMAGE) $(METADATA) FORCE
	srec_cat \
		$(BOOTLOADER) -intel -crop 0x00000000 0x00002000 0x0001FFA8 0x00020000 \
		$(METADATA) -intel -crop 0x0001FF98 0x0001FFA8 \
		$(GOLDEN_IMAGE) -intel -crop 0x00002000 0x0001FF98 \
		-o $@ -intel
