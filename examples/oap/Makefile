### At the moment this example only supports cc26x0-cc13x0. So make sure that
### a simple `make` will try to do something meaningful
TARGET ?= cc26x0-cc13x0
BOARD ?= launchpad/cc2650

ifneq ($(TARGET), cc26x0-cc13x0)
$(error This example does not support platform $(TARGET))
endif

BOOTLOADER = bootloader/bootloader.hex
GOLDEN_IMAGE = golden-image/golden-image.hex
METADATA = metadata.hex
FINAL_IMAGE = image.hex
GENERATE_METADATA = ../../tools/oap/generate-metadata.py

# Handle debugging ability
ifeq ($(BOOTLOADER_DEBUG), 1)
  BOOTLOADER_END_ADDR = 0x00003000
endif

export BOOTLOADER_DEBUG

METADATA_OFFSET = 0x0001FF9C
CCFG_START_ADDR = 0x0001FFA8
CCFG_END_ADDR = 0x00020000
BOOTLOADER_START_ADDR = 0x00000000
BOOTLOADER_END_ADDR ?= 0x00002000
BOOTLOADER_CROP_REGIONS += $(BOOTLOADER_START_ADDR) $(BOOTLOADER_END_ADDR)
BOOTLOADER_CROP_REGIONS += $(CCFG_START_ADDR) $(CCFG_END_ADDR)
GOLDEN_IMAGE_CROP_REGIONS = $(BOOTLOADER_END_ADDR) $(METADATA_OFFSET)
METADATA_CROP_REGIONS = $(METADATA_OFFSET) $(CCFG_START_ADDR)

all: $(FINAL_IMAGE)

.PHONY: clean distclean

mostlyclean:
	rm -f $(FINAL_IMAGE) $(METADATA)

clean: mostlyclean
	$(MAKE) -C $(dir $(BOOTLOADER)) TARGET=$(TARGET) clean
	$(MAKE) -C $(dir $(GOLDEN_IMAGE)) TARGET=$(TARGET) clean

distclean: mostlyclean
	$(MAKE) -C $(dir $(BOOTLOADER)) distclean
	$(MAKE) -C $(dir $(GOLDEN_IMAGE)) distclean

FORCE:

$(BOOTLOADER): FORCE
	$(MAKE) -C $(dir $@) TARGET=$(TARGET) BOARD=$(BOARD) $(notdir $@)

$(GOLDEN_IMAGE): FORCE
	$(MAKE) -C $(dir $@) TARGET=$(TARGET) BOARD=$(BOARD) $(notdir $@)

$(METADATA): $(GOLDEN_IMAGE)
	python $(GENERATE_METADATA) -o $@ -O $(METADATA_OFFSET) $<

$(FINAL_IMAGE): $(BOOTLOADER) $(GOLDEN_IMAGE) $(METADATA) FORCE
	srec_cat \
		$(BOOTLOADER) -intel -crop $(BOOTLOADER_CROP_REGIONS) \
		$(METADATA) -intel -crop $(METADATA_CROP_REGIONS) \
		$(GOLDEN_IMAGE) -intel -crop $(GOLDEN_IMAGE_CROP_REGIONS) \
		-o $@ -intel
