# Standalone native NAT64 test.
#
# One check, reported via the summary/"TEST FAIL" convention that
# tests/check-test.sh expects: build a native uIP UDP probe with the standalone
# NAT64 module (os/services/nat64/native/standalone), send to a 64:ff9b::
# address mapping 127.0.0.1, and check it is echoed back through the
# translator's host sockets. Needs no Cooja, border router, tun, sudo, or
# internet.
#
# Author: Nicolas Tsiftes <nicolas.tsiftes@ri.se>

SHELL := /bin/bash

CONTIKI ?= ../..

NAT64_TEST := $(CURDIR)/test-nat64-standalone.sh

.PHONY: all summary nat64-standalone clean

all: summary

# A successful check leaves no summary line; the final rule then creates an
# empty summary so check-test.sh (which greps for "TEST FAIL") sees a pass.
summary: clean
	@$(MAKE) nat64-standalone
	@echo "========== Summary =========="
	@([ ! -f summary ] && echo "All tests OK" && touch summary) || \
          (echo "Failures:" && cat summary && false)

nat64-standalone:
	@echo ">> standalone NAT64 native round-trip"
	@$(NAT64_TEST) || echo "TEST FAIL: nat64-standalone-native" >> summary

clean:
	@rm -f *.*log *.pid report summary
	@$(MAKE) -C code-nat64-standalone TARGET=native clean >/dev/null 2>&1 || true
