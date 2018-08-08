# Makefile used to generate documentation with sphinx, which is used on
# readthedocs. Use this Makefile to test readthedocs generation locally on
# your computer.
#
# For this to work, you will need to install sphinx and sphinx_rtd_theme. They
# can be installed through pip
.PHONY = clean all readthedocs

SPHINX = sphinx-build
SPHINX_CONF_DIR = .
SPHINX_OUT_DIR = _build

all: readthedocs

clean:
	rm -rf $(SPHINX_OUT_DIR)

readthedocs:
	$(SPHINX) -b html $(SPHINX_CONF_DIR) $(SPHINX_OUT_DIR)
