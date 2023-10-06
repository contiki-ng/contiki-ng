RUN_FILE = 1

EXAMPLESDIR := $(abspath ../..)

EXAMPLES = \
examples/lwm2m-ipso-objects/native:./06-lwm2m-ipso-test.sh \
tests/18-coap-lwm2m/example-lwm2m-standalone/lwm2m/native:./07-lwm2m-standalone-test.sh:CONTIKI_NG=$(EXAMPLESDIR) \
examples/lwm2m-ipso-objects/native:./08-lwm2m-qmode-ipso-test.sh:DEFINES=LWM2M_QUEUE_MODE_CONF_ENABLED=1,LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1,LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED=1 \
tests/18-coap-lwm2m/example-lwm2m-standalone/lwm2m/native:./09-lwm2m-qmode-standalone-test.sh:CONTIKI_NG=$(EXAMPLESDIR):DEFINES=LWM2M_QUEUE_MODE_CONF_ENABLED=1,LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1,LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED=1

include ../Makefile.compile-test
