ifndef HOST_OS
  ifeq ($(OS),Windows_NT)
    HOST_OS := Windows
  else
    HOST_OS := $(shell uname)
  endif
endif

ifeq ($(HOST_OS),Windows)
  SERIALDUMP = serialdump-windows
endif

ifeq ($(HOST_OS),Darwin)
  SERIALDUMP = serialdump-macos
endif

ifndef SERIALDUMP
  # Assume Linux
  SERIALDUMP = serialdump-linux
endif

all:	$(SERIALDUMP)

CFLAGS += -Wall -Werror

$(SERIALDUMP):	serialdump.c
	$(CC) -O2 -o $@ $<

clean:
	rm -f $(SERIALDUMP)
