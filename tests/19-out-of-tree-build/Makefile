EXAMPLESDIR=$(HOME)/out-of-tree-tests
TOOLSDIR=$(HOME)/contiki-ng/tools

EXAMPLES = \
hello-world/native \
hello-world/native:MAKE_NET=MAKE_NET_NULLNET \
hello-world/native:MAKE_ROUTING=MAKE_ROUTING_RPL_CLASSIC \
hello-world/sky \
hello-world/nrf52dk \
hello-world/cc2538dk \
hello-world/zoul \
hello-world/openmote \
hello-world/cc26x0-cc13x0 \
hello-world/jn516x \
hello-world/simplelink:BOARD=launchpad/cc26x2r1 \
hello-world/simplelink:BOARD=sensortag/cc2650 \

TOOLS=

include ../Makefile.compile-test
