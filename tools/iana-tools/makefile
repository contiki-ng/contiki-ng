# Default target
all: generate

# Create a virtual environment
venv: venv/touchfile
venv/touchfile: ./iana-headers/requirements.txt
	@echo "Installing dependencies"
	test -d venv || python3 -m venv venv
	. venv/bin/activate; venv/bin/pip install -r ./iana-headers/requirements.txt
	touch venv/touchfile

# Generate headers
.PHONY: generate
generate: venv
	@echo "Generating Headers"
	. venv/bin/activate; cd ./iana-headers/c; ./c_header_coap.py --settings ../../iana_settings_contiki.toml

# Clean
.PHONY: clean
clean:
	@echo "Cleaning generated files"
	rm -rf ./iana-headers/c/cache/*

# Help target
.PHONY: help
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  install     : Create virtual environment and install dependencies"
	@echo "  generate    : Generate headers"
	@echo "  clean       : Clean generated files"
	@echo "  help        : Display this help message"
