ifeq ($(TARGET),)
    $(error TARGET is not set)
endif
ifeq ($(BOARD),)
    $(error BOARD is not set)
endif

CONTIKI=../..

SECURE_SIZE_BIN=secure_size.$(TARGET).$(BOARD)

all: $(SECURE_SIZE_BIN)

$(SECURE_SIZE_BIN): secure_size.c
	gcc -o $@ $^ -I$(CONTIKI)/arch/platform/$(TARGET)/$(BOARD)
