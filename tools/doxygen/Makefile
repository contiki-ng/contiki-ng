# Doxyfile configuration variables
export doclatex	:= NO
export docroot	:= ../..
export PATH     := .:$(PATH)

.PHONY: clean html

html: clean
	@doxygen Doxyfile

clean:
	@echo "> Cleaning Documentation"
	@$(RM) -r html doxygen.log
