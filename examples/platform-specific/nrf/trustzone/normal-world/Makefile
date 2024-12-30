CONTIKI_PROJECT = normal-world-example

all: $(CONTIKI_PROJECT)

PLATFORMS_ONLY += nrf
BOARDS_ONLY = nrf5340/dk/application

TARGET ?= nrf
BOARD ?= nrf5340/dk/application

TRUSTZONE_SECURE_BUILD = 0
TRUSTZONE_SECURE_WORLD_PATH ?= ../secure-world

include ../../../../../Makefile.include
