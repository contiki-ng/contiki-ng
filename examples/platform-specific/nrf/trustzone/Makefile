# Hard-wired settings for the nRF5340.
BUILD_DIR ?= build/nrf/nrf5340/dk/application
SREC_CAT ?= srec_cat

# Default firmware names. The first two are the names of the secure world
# and normal world firmwares, which are used as inputs to concatenate into
# the MERGED_FIRMWARE.
SECURE_WORLD_FIRMWARE ?= secure-world-example
NORMAL_WORLD_FIRMWARE ?= normal-world-example
MERGED_FIRMWARE ?= tz-merged

.PHONY: all
all: tz_build

# Build a merged firmware of the normal world and the secure world,
# both of which are running Contiki-NG with different configurations.
.PHONY: tz_build
tz_build:
	$(MAKE) -C secure-world $(BUILD_DIR)/$(SECURE_WORLD_FIRMWARE).hex
	$(MAKE) -C normal-world $(BUILD_DIR)/$(NORMAL_WORLD_FIRMWARE).hex
	$(SREC_CAT) secure-world/$(BUILD_DIR)/$(SECURE_WORLD_FIRMWARE).hex -Intel \
		normal-world/$(BUILD_DIR)/$(NORMAL_WORLD_FIRMWARE).hex -Intel \
		-o secure-world/$(BUILD_DIR)/$(MERGED_FIRMWARE).hex -Intel

# Upload the merged firmware to an IoT device.
.PHONY: upload
upload: tz_build
	$(MAKE) -C secure-world $(MERGED_FIRMWARE).upload PORT=$(PORT)

.PHONY: login
login:
	$(MAKE) -C secure-world login PORT=$(PORT)

.PHONY: clean
clean:
	$(MAKE) -C secure-world clean
	$(MAKE) -C normal-world clean
