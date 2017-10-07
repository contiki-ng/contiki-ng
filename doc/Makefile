basedirs	:= os arch
manuals		:= $(shell find ./ -name "*.txt")
filetypes	:= -iwholename "*/doc/*.txt"
empty		:=
space		:= $(empty) $(empty)
pwd			:= $(shell cd ..; pwd)

# Doxyfile configuration variables
export docdir	:= .
export docdirs
export docsrc
export doclatex	:= NO
export docroot	:= ../

# Get appropriate root for doxygen path cutoff
ifeq ($(HOST_OS),Windows)
  # on windows need to convert cygwin path to windows path for doxygen
  ifneq (,$(findstring cygdrive,$(pwd)))
    cygroot = $(subst /,$(space),$(patsubst /cygdrive/%,%,$(pwd)))
    export docroot = $(firstword $(cygroot)):/$(subst $(space),/,$(wordlist 2,$(words $(cygroot)),$(cygroot)))
  endif
endif

docdirs = $(sort $(foreach dir,$(basedirs),${shell find ../${dir} -type d -not -path "*/.*" -not -path "*/obj_*"}))
docsrc = $(docdirs) $(foreach dir,$(docdirs),${shell find $(dir) -type f $(filetypes)}) $(manuals)

.PHONY: clean html

html:
	@doxygen Doxyfile

clean:
	@echo "> Cleaning Documentation"
	@$(RM) -r "$(docdir)/html"
	@$(RM) -r "doxygen.log"
	@echo " done."
