# Code style

## Naming

* File names are composed of lower-case characters and dashes. Like
  this: `simple-udp.c`
* Variable and function names are composed of lower-case characters
  and underscores. Like this: `simple_udp_send()`
* Variable and function names that are visible outside of their module
  must begin with the name of the module. Like this:
  `simple_udp_send()`, which is in the `simple-udp` module, declared in
  `simple-udp.h`, and implemented in `simple-udp.c`
* C macros are composed of upper-case characters and underscores. Like
  this: `PROCESS_THREAD()`
* Configuration definitions begin with the module name and CONF_. Like
  this: `PROCESS_CONF_NUMEVENTS`

## Formatting scripts

The Contiki source tree contains scripts to assist with correct code formatting
and we recommend [Uncrustify](http://uncrustify.sourceforge.net/) as the
preferred auto formatter. Everything is under
[tools/code-style](https://github.com/contiki-ng/contiki-ng/tree/master/tools/code-style).

If you wish, you can format all changed resources in your working tree
automatically if the
[tools/code-style/uncrustify-changed.sh](https://github.com/contiki-ng/contiki-ng/blob/master/tools/code-style/uncrustify-changed.sh)
script is added as a [Git pre-commit
hook](http://git-scm.com/book/en/Customizing-Git-Git-Hooks) to your Git
configuration.

Here are some examples of what you can do:
* To check a file's style without changing the file on disk, you can run this:
`./tools/code-style/uncrustify-check-style.sh <path-to-file>`
This script will only accept a single file as its argument.

* To auto format a file (and change it on disk) you can run this:
`./tools/code-style/uncrustify-fix-style.sh <path-to-file>`

* `uncrustify-fix-style.sh` will accept a space-delimited list of files as its argument. Thus, you can auto-format an entire directory by running something like this:
``./tools/code-style/uncrustify-fix-style.sh `find arch/cpu/cc2538 -type f -name "*.[ch]"` ``

This is _not_ a silver bullet and developer intervention is still required. Below are some examples of code which will get misformatted by uncrustify:
* Math symbol following a cast to a typedef
```c
  a = (uint8_t) ~P0_1; /* Cast to a typedef. Space gets added here (incorrect) */
  a = (int)~P0_1;      /* Cast to a known type. Space gets removed (correct) */
  a = (uint8_t)P0_1;   /* Variable directly after the cast. Space gets removed (correct) */
```

* `while(<condition>);` will become `while(<condition>) ;` (space incorrectly added after closing paren)

* ﻿﻿`asm("wfi");` becomes `asm ("wfi");`: A space gets added before the opening paren, because the `asm` keyword stops this from getting interpreted as a normal function call / macro invocation. This is only a problem with `asm`. For instance, ﻿﻿`foo("bar");` gets formatted correctly.

## Example file

Below is an example .c files that complies with the Contiki-NG code style:
```c
/**
 * \defgroup coding-style Coding style
 *
 * This is how a Doxygen module is documented - start with a \defgroup
 * Doxygen keyword at the beginning of the file to define a module,
 * and use the \addtogroup Doxygen keyword in all other files that
 * belong to the same module. Typically, the \defgroup is placed in
 * the .h file and \addtogroup in the .c file.
 *
 * The Contiki source code contains an Uncrustify configuration file,
 * uncrustify.cfg, under tools/code-style and small helper scripts are
 * provided at the same place. Note that this is not a silver bullet -
 * for example, the script does not add separators between functions,
 * nor does it format comments correctly. The script should be treated
 * as an aid in formatting code: first run the formatter on the source
 * code, then manually edit the file.
 *
 * @{
 */

/**
 * \file
 *         A brief description of what this file is.
 * \author
 *         <Author's name> <Author's e-mail address>
 *
 *         Every file that is part of a documented module has to have
 *         a \file block, else it will not show up in the Doxygen
 *         "Modules" * section.
 */

/* Single line comments look like this. */

/*
 * Multi-line comments look like this. Comments should prefferably be
 * full sentences, filled to look like real paragraphs.
 */

#include "contiki.h"

/*
 * Make sure that non-global variables are all maked with the static
 * keyword. This keeps the size of the symbol table down.
 */
static int flag;

/*
 * All variables and functions that are visible outside of the file
 * should have the module name prepended to them. This makes it easy
 * to know where to look for function and variable definitions.
 *
 * Put dividers (a single-line comment consisting only of dashes)
 * between functions.
 */
/*---------------------------------------------------------------------------*/
/**
 * \brief      Use Doxygen documentation for functions.
 * \param c    Briefly describe all parameters.
 * \return     Briefly describe the return value.
 * \retval 0   Functions that return a few specified values
 * \retval 1   can use the \retval keyword instead of \return.
 *
 *             Put a longer description of what the function does
 *             after the preamble of Doxygen keywords.
 *
 *             This template should always be used to document
 *             functions. The text following the introduction is used
 *             as the function's documentation.
 *
 *             Function prototypes have the return type on one line,
 *             the name and arguments on one line (with no space
 *             between the name and the first parenthesis), followed
 *             by a single curly bracket on its own line.
 */
int
code_style_example_function(char c)
{
  /*
   * There should be no space between keywords and the first
   * parenthesis. There should be spaces around binary operators, no
   * spaces between a unary operator and its operand.
   *
   * Curly brackets following for(), if(), do, and switch() statements
   * should follow the statement on the same line.
   *
   * Use short variable names for loop counters.
   */
  for(int i = 0; i < 10; ++i) {

    /*
     * Declare variables as locally as possible.
     */
    int sum = i + c;

    /*
     * Always use full blocks (curly brackets) after if(), for(), and
     * while() statements, even though the statement is a single line
     * of code. This makes the code easier to read and modifications
     * are less error prone.
     */
    if(i == c) {
      return sum;         /* No parenthesis around return values. */
    } else {              /* The else keyword is placed inbetween
                             curly braces, always on its own line. */
      c++;
    }

    c += sum;
  }

  /* Do not indent case and default within a switch block */
  switch(c) {
  case 19:
    return 1;
  default:
    break;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Static (non-global) functions do not need Doxygen comments. The
 * name should not be prepended with the module name - doing so would
 * create confusion.
 */
static void
an_example_function(void)
{

}
/*---------------------------------------------------------------------------*/

/* The following stuff ends the \defgroup block at the beginning of
   the file: */

/** @} */
```
