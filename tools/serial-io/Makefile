APPS = tunslip6 serialdump
LIB_SRCS = tools-utils.c
DEPEND = tools-utils.h

all: $(APPS)

CFLAGS += -Wall -Werror

ifeq ("(DEBUG)","")
CFLAGS += -O2
else
CFLAGS += -O1 -g3
endif


$(APPS) : % : %.c $(LIB_SRCS) $(DEPEND)
	$(CC) $(CFLAGS) $< $(LIB_SRCS) -o $@

clean:
	rm -f $(APPS)
