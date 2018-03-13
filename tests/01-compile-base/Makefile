EXAMPLESDIR=../../examples
TOOLSDIR=../../tools

EXAMPLES = \
hello-world/native \
hello-world/native:MAKE_NET=MAKE_NET_NULLNET \
hello-world/native:MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC \
hello-world/sky \
storage/eeprom-test/native \
multicast/sky \
libs/logging/native \
libs/energest/native \
libs/energest/sky \
libs/data-structures/native \
libs/data-structures/sky \
libs/stack-check/sky \
ipso-objects/native \
ipso-objects/native:MAKE_WITH_DTLS=1 \
ipso-objects/native:DEFINES=LWM2M_Q_MODE_CONF_ENABLED=1 \
ipso-objects/native:DEFINES=LWM2M_Q_MODE_CONF_ENABLED=1,LWM2M_Q_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1\
rpl-udp/sky \
rpl-border-router/native \
rpl-border-router/sky \
slip-radio/sky \
ipv6-hooks/sky \
nullnet/native \

TOOLS=

include ../Makefile.compile-test
