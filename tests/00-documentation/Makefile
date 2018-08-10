# Copyright (c) 2014, Friedrich-Alexander University Erlangen-Nuremberg
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
TOOLS_DIR = ../../tools

DOXYGEN = doxygen
DOXYGEN_DIR = $(TOOLS_DIR)/doxygen
DOXYGEN_LOG = $(DOXYGEN).log
DOXYGEN_ERR = $(DOXYGEN).err

RAEDTHEDOCS = readthedocs
READTHEDOCS_DIR = $(TOOLS_DIR)/readthedocs
READTHEDOCS_LOG = $(RAEDTHEDOCS).log
READTHEDOCS_ERR = $(RAEDTHEDOCS).err

CLEAN_TARGETS += $(DOXYGEN_LOG) $(DOXYGEN_ERR)
CLEAN_TARGETS += $(READTHEDOCS_LOG) $(READTHEDOCS_ERR)

all: clean summary

doxygen:
	-@$(MAKE) -C $(DOXYGEN_DIR) 2> $(DOXYGEN_ERR) > /dev/null

readthedocs:
	-@$(MAKE) -C $(READTHEDOCS_DIR) 2> $(READTHEDOCS_ERR) > /dev/null

summary: doxygen readthedocs
	@( \
		1> summary; \
		if [ -s $(DOXYGEN_ERR) ] ; then \
			echo "Doxygen: TEST FAIL" | tee -a summary; \
			echo "Errors:"; \
			cat $(DOXYGEN_ERR); \
		fi ; \
		if [ -s $(DOXYGEN_DIR)/doxygen.log ] ; then \
			echo "Doxygen: TEST FAIL" | tee -a summary; \
			echo "Warnings:"; \
			cat $(DOXYGEN_DIR)/doxygen.log; \
		fi ; \
		if [ -s $(READTHEDOCS_ERR) ] ; then \
			echo "Readthedocs: TEST FAIL" | tee -a summary; \
			echo "Errors:"; \
			cat $(READTHEDOCS_ERR); \
		fi ; \
		if [ ! -s summary ] ; then \
			echo "Documentation: TEST OK (no warning nor error)" | tee summary; \
		fi ; \
	)
	@rm -f $(CLEAN_TARGETS)
	@echo "========== Summary =========="
	@cat summary

clean:
	@rm -f summary $(CLEAN_TARGETS)
	@$(MAKE) -C $(DOXYGEN_DIR) clean
	@$(MAKE) -C $(READTHEDOCS_DIR) clean
