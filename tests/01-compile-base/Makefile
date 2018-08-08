EXAMPLESDIR=../../examples
TOOLSDIR=../../tools

EXAMPLES = \
hello-world/native \
hello-world/native:MAKE_NET=MAKE_NET_NULLNET \
hello-world/native:MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC \
hello-world/sky \
storage/eeprom-test/native \
libs/logging/native \
libs/energest/native \
libs/energest/sky \
libs/data-structures/native \
libs/data-structures/sky \
libs/stack-check/sky \
lwm2m-ipso-objects/native \
lwm2m-ipso-objects/native:MAKE_WITH_DTLS=1 \
lwm2m-ipso-objects/native:DEFINES=LWM2M_Q_MODE_CONF_ENABLED=1 \
lwm2m-ipso-objects/native:DEFINES=LWM2M_Q_MODE_CONF_ENABLED=1,LWM2M_Q_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1\
rpl-udp/sky \
rpl-border-router/native \
rpl-border-router/native:MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC \
rpl-border-router/sky \
slip-radio/sky \
libs/ipv6-hooks/sky \
nullnet/native \
mqtt-client/native \
coap/coap-example-client/native \
coap/coap-example-server/native \
coap/coap-plugtest-server/native \

TOOLS=

include ../Makefile.compile-test
