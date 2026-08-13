CONTIKI_PROJECT = test-dbg-io
all: $(CONTIKI_PROJECT)

TARGET = native

MODULES += os/services/unit-test
MODULES += os/lib/dbg-io

CONTIKI = ../../..

# Prevent the compiler from replacing our dbg-io implementations
# with built-in versions from libc.
CFLAGS += -fno-builtin-printf -fno-builtin-puts -fno-builtin-putchar
CFLAGS += -fno-builtin-sprintf -fno-builtin-snprintf
CFLAGS += -fno-builtin-vsnprintf

include $(CONTIKI)/Makefile.include
