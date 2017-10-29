contiking := ../..
basedirs	:= os arch
empty		:=
space		:= $(empty) $(empty)
pwd			:= $(shell cd $(contiking); pwd)

# Doxyfile configuration variables
export docdir	:= .
export doclatex	:= NO
export docroot	:= $(contiking)

# Get appropriate root for doxygen path cutoff
ifeq ($(HOST_OS),Windows)
  # on windows need to convert cygwin path to windows path for doxygen
  ifneq (,$(findstring cygdrive,$(pwd)))
    cygroot = $(subst /,$(space),$(patsubst /cygdrive/%,%,$(pwd)))
    export docroot = $(firstword $(cygroot)):/$(subst $(space),/,$(wordlist 2,$(words $(cygroot)),$(cygroot)))
  endif
endif

export docdirs = $(sort $(foreach dir,$(basedirs),${shell find $(contiking)/${dir} -type d}))
export docsrc = $(docdirs) $(foreach dir,$(docdirs) .,${shell find $(dir) -type f -name "*.txt"})

.PHONY: clean html

html: clean
	@doxygen Doxyfile

clean:
	@echo "> Cleaning Documentation"
	@$(RM) -r "$(docdir)/html"
	@$(RM) -r "doxygen.log"
	@echo " done."
