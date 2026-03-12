# TrustZone wrapper for the rpl-udp example on nRF54L15.
BOARD ?= nrf54l15/xiao
BUILD_DIR ?= build/nrf/$(BOARD)
SREC_CAT ?= srec_cat

APP ?= udp-client
VALID_APPS := udp-client udp-server

ifeq (,$(filter $(APP),$(VALID_APPS)))
$(error APP must be one of: $(VALID_APPS))
endif

SECURE_WORLD_FIRMWARE ?= secure-world-example
NORMAL_WORLD_FIRMWARE ?= $(APP)
MERGED_FIRMWARE ?= tz-rpl-udp-$(APP)

.PHONY: all
all: tz_build

.PHONY: tz_build
tz_build:
	$(MAKE) -C secure-world BOARD=$(BOARD) $(BUILD_DIR)/$(SECURE_WORLD_FIRMWARE).hex
	$(MAKE) -C normal-world BOARD=$(BOARD) CONTIKI_PROJECT=$(APP) $(BUILD_DIR)/$(NORMAL_WORLD_FIRMWARE).hex
	$(SREC_CAT) secure-world/$(BUILD_DIR)/$(SECURE_WORLD_FIRMWARE).hex -Intel \
		normal-world/$(BUILD_DIR)/$(NORMAL_WORLD_FIRMWARE).hex -Intel \
		-o secure-world/$(BUILD_DIR)/$(MERGED_FIRMWARE).hex -Intel

.PHONY: upload
upload: tz_build
	$(MAKE) -C secure-world BOARD=$(BOARD) $(MERGED_FIRMWARE).upload PORT=$(PORT)

.PHONY: login
login:
	$(MAKE) -C secure-world BOARD=$(BOARD) login PORT=$(PORT)

.PHONY: clean
clean:
	$(MAKE) -C secure-world clean
	$(MAKE) -C normal-world clean
