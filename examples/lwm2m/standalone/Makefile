PROJECT = lwm2m-example

all:	$(PROJECT)

TARGETCDIR   = lwm2m-src
COREDIRS     = sys lib
SOURCEDIRS   = .
SOURCE_FILES = posix-main.c posix-coap-timer.c ipso-sensor-temp.c \
	       ipso-control-test.c generic-object-test.c
APPSDIRS     = coap lwm2m ipso-objects

ifneq ($(MAKE_WITH_DTLS),)
  CFLAGS += -DWITH_DTLS=1
  TINYDTLS_DIRS = aes ecc sha2
  APPSDIRS += tinydtls ${addprefix tinydtls/,$(TINYDTLS_DIRS)}
  SOURCE_FILES += dtls-support.c
  SOURCEDIRS += tinydtls-support
endif

TARGETCDIRS  += ${addprefix $(TARGETCDIR)/,$(COREDIRS) $(APPSDIRS)}
SOURCEDIRS   += ${addprefix $(TARGETCDIR)/,. $(APPSDIRS)}

CFLAGS += -Wall -Werror
CFLAGS += ${addprefix -I,$(SOURCEDIRS)}

TRANSPORT ?= ipv4
ifeq ($(TRANSPORT),ipv4)
  TRANSPORTDIR = coap-ipv4
  CFLAGS += -DWITH_COAP_IPV4=1 -DCOAP_TRANSPORT_CONF_H=\"coap-ipv4.h\"
else ifeq ($(TRANSPORT),hex)
  TRANSPORTDIR = coap-hex
  CFLAGS += -DWITH_COAP_HEX=1 -DCOAP_TRANSPORT_CONF_H=\"coap-hex.h\"
else
  ${error Unknown CoAP transport: $(TRANSPORT)}
endif

SOURCEDIRS += $(TRANSPORTDIR)
SOURCE_FILES += ${notdir ${wildcard $(TRANSPORTDIR)/*.c}}

vpath %.c $(SOURCEDIRS)

-include Makefile.contiki

ifndef OBJECT_FILES
 SOURCE_FILES += ${wildcard ${addsuffix /*.c,$(TARGETCDIRS)} }
 OBJECT_FILES = $(SOURCE_FILES:.c=.o)
endif

$(PROJECT): $(PROJECT).c $(OBJECT_FILES)
	$(CC) $(CFLAGS) -o $@ $(PROJECT).c $(OBJECT_FILES)

lib:	liblwm2m.a

clean::
	@-rm -f $(OBJECT_FILES) *.o liblwm2m.a $(PROJECT)

liblwm2m.a: $(OBJECT_FILES)
	$(AR) rvs $@ $(OBJECT_FILES)

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@
