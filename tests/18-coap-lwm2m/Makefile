RUN_FILE = 1

EXAMPLESDIR := $(abspath ../..)

EXAMPLES = \
examples/lwm2m-ipso-objects/native:./06-lwm2m-ipso-test.sh \
tests/18-coap-lwm2m/example-lwm2m-standalone/lwm2m/native:./07-lwm2m-standalone-test.sh:CONTIKI_NG=$(EXAMPLESDIR) \
examples/lwm2m-ipso-objects/native:./08-lwm2m-qmode-ipso-test.sh:DEFINES=LWM2M_QUEUE_MODE_CONF_ENABLED=1,LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1,LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED=1 \
tests/18-coap-lwm2m/example-lwm2m-standalone/lwm2m/native:./09-lwm2m-qmode-standalone-test.sh:CONTIKI_NG=$(EXAMPLESDIR):DEFINES=LWM2M_QUEUE_MODE_CONF_ENABLED=1,LWM2M_QUEUE_MODE_CONF_INCLUDE_DYNAMIC_ADAPTATION=1,LWM2M_QUEUE_MODE_OBJECT_CONF_ENABLED=1 \
examples/lwm2m-ipso-objects/native:./10-lwm2m-ipso-dtls-test.sh:MAKE_WITH_DTLS=1:MAKE_COAP_DTLS_WITH_PSK=1:MAKE_COAP_DTLS_WITH_CLIENT=1:MAKE_COAP_DTLS_KEYSTORE=MAKE_COAP_DTLS_KEYSTORE_SIMPLE:DEFINES=LWM2M_CONF_ENGINE_CLIENT_ENDPOINT_NAME=\\\"Contiki-NG-IPSO\\\" \
examples/lwm2m-ipso-objects/native:./10-lwm2m-ipso-dtls-test.sh:MAKE_WITH_DTLS=1:MAKE_COAP_DTLS_WITH_CERT=1:MAKE_COAP_DTLS_WITH_CLIENT=1:MAKE_COAP_DTLS_KEYSTORE=MAKE_COAP_DTLS_KEYSTORE_SIMPLE

include ../Makefile.compile-test
