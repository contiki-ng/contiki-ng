#!/bin/sh -e
##############################################################################
# This script builds mspgcc using gcc 4.7.4 as the basis.                    #
#                                                                            #
# This script is a modified version (by Daniele Alessandrelli) of the script #
# created by Hossein Shafagh and hosted here:                                #
# http://wiki.contiki-os.org/doku.php?id=msp430x                             #
#                                                                            #
# The script has been further modified (J. Brusey) to patch binutils and     #
# gcc for texinfo version 5 and to check that the install directory          #
# exists and is writable.                                                    #
#                                                                            #
# There are several issues outstanding. Search for FIXME in this script.     #
#                                                                            #
# By default the compiler is installed in `/usr/local/stow/mspgcc-4.7.4`     #
# so that this install can be used with the `stow` package. You can          #
# change this by modifying INSTALL_PREFIX.                                   #
#                                                                            #
# Instructions:                                                              #
#                                                                            #
# 1. edit the script at `tools/toolchain/msp430/buildmsp.sh` to update       #
#    the INSTALL_PREFIX according to your preferred install location.        #
#                                                                            #
# 2. run `tools/toolchain/msp430/buildmsp.sh` from the command line. It      #
#    should not be necessary to use sudo but ensure that you have write      #
#    access to the INSTALL_PREFIX directory.                                 #
##############################################################################

INSTALL_PREFIX="/usr/local/stow/mspgcc-4.7.4"
echo The installation prefix is: $INSTALL_PREFIX

error() {
    echo "Error: $*"
    exit 1
}

[ -d $INSTALL_PREFIX ] || error "Directory does not exist: $INSTALL_PREFIX"

[ -w $INSTALL_PREFIX ] || error "No write permission to: $INSTALL_PREFIX"

tmpdir=$(mktemp -d)
echo "Creating temporary directory: $tmpdir"
exithandler() {
    if [ $? -eq 0 ]; then
	echo "Build successful - removing temporary directory $tmpdir"
	rm -rf $tmpdir
    else
	echo "Build failed - $tmpdir not removed"
    fi
}
trap 'exithandler' EXIT
# Switch to the temporary directory
cd $tmpdir
# Getting
wget --no-verbose http://sourceforge.net/projects/mspgcc/files/mspgcc/DEVEL-4.7.x/mspgcc-20120911.tar.bz2
wget --no-verbose http://sourceforge.net/projects/mspgcc/files/msp430mcu/msp430mcu-20120716.tar.bz2
wget --no-verbose http://sourceforge.net/projects/mspgcc/files/msp430-libc/msp430-libc-20120716.tar.bz2
wget --no-verbose http://ftpmirror.gnu.org/binutils/binutils-2.22.tar.bz2
wget --no-verbose http://ftpmirror.gnu.org/gcc/gcc-4.7.4/gcc-4.7.4.tar.bz2
wget --no-verbose http://sourceforge.net/p/mspgcc/bugs/352/attachment/0001-SF-352-Bad-code-generated-pushing-a20-from-stack.patch
wget --no-verbose  http://sourceforge.net/p/mspgcc/bugs/_discuss/thread/fd929b9e/db43/attachment/0001-SF-357-Shift-operations-may-produce-incorrect-result.patch

# Unpacking the tars
tar xfj binutils-2.22.tar.bz2
tar xfj gcc-4.7.4.tar.bz2
tar xfj mspgcc-20120911.tar.bz2
tar xfj msp430mcu-20120716.tar.bz2
tar xfj msp430-libc-20120716.tar.bz2

# 1) Incorporating the changes contained in the patch delivered in mspgcc-20120911
cd binutils-2.22
patch -p1<../mspgcc-20120911/msp430-binutils-2.22-20120911.patch
#    This patch is for texinfo 5
patch -p1 <<EOF
diff --git c/gas/doc/c-arc.texi i/gas/doc/c-arc.texi
index 3a136a7..cd6f0d9 100644
--- c/gas/doc/c-arc.texi
+++ i/gas/doc/c-arc.texi
@@ -212,7 +212,7 @@ The extension instructions are not macros.  The assembler creates
 encodings for use of these instructions according to the specification
 by the user.  The parameters are:

-@table @bullet
+@table @code
 @item @var{name}
 Name of the extension instruction

diff --git c/gas/doc/c-arm.texi i/gas/doc/c-arm.texi
index d3cccf4..97c2f92 100644
--- c/gas/doc/c-arm.texi
+++ i/gas/doc/c-arm.texi
@@ -376,29 +376,29 @@ ARM and THUMB instructions had their own, separate syntaxes.  The new,
 @code{unified} syntax, which can be selected via the @code{.syntax}
 directive, and has the following main features:

-@table @bullet
-@item
+@table @code
+@item 1
 Immediate operands do not require a @code{#} prefix.

-@item
+@item 2
 The @code{IT} instruction may appear, and if it does it is validated
 against subsequent conditional affixes.  In ARM mode it does not
 generate machine code, in THUMB mode it does.

-@item
+@item 3
 For ARM instructions the conditional affixes always appear at the end
 of the instruction.  For THUMB instructions conditional affixes can be
 used, but only inside the scope of an @code{IT} instruction.

-@item
+@item 4
 All of the instructions new to the V6T2 architecture (and later) are
 available.  (Only a few such instructions can be written in the
 @code{divided} syntax).

-@item
+@item 5
 The @code{.N} and @code{.W} suffixes are recognized and honored.

-@item
+@item 6
 All instructions set the flags if and only if they have an @code{s}
 affix.
 @end table
@@ -433,28 +433,6 @@ Either @samp{#} or @samp{$} can be used to indicate immediate operands.
 @cindex register names, ARM
 *TODO* Explain about ARM register naming, and the predefined names.

-@node ARM-Neon-Alignment
-@subsection NEON Alignment Specifiers
-
-@cindex alignment for NEON instructions
-Some NEON load/store instructions allow an optional address
-alignment qualifier.
-The ARM documentation specifies that this is indicated by
-@samp{@@ @var{align}}. However GAS already interprets
-the @samp{@@} character as a "line comment" start,
-so @samp{: @var{align}} is used instead.  For example:
-
-@smallexample
-        vld1.8 @{q0@}, [r0, :128]
-@end smallexample
-
-@node ARM Floating Point
-@section Floating Point
-
-@cindex floating point, ARM (@sc{ieee})
-@cindex ARM floating point (@sc{ieee})
-The ARM family uses @sc{ieee} floating-point numbers.
-
 @node ARM-Relocations
 @subsection ARM relocation generation

@@ -497,6 +475,28 @@ respectively.  For example to load the 32-bit address of foo into r0:
         MOVT r0, #:upper16:foo
 @end smallexample

+@node ARM-Neon-Alignment
+@subsection NEON Alignment Specifiers
+
+@cindex alignment for NEON instructions
+Some NEON load/store instructions allow an optional address
+alignment qualifier.
+The ARM documentation specifies that this is indicated by
+@samp{@@ @var{align}}. However GAS already interprets
+the @samp{@@} character as a "line comment" start,
+so @samp{: @var{align}} is used instead.  For example:
+
+@smallexample
+        vld1.8 @{q0@}, [r0, :128]
+@end smallexample
+
+@node ARM Floating Point
+@section Floating Point
+
+@cindex floating point, ARM (@sc{ieee})
+@cindex ARM floating point (@sc{ieee})
+The ARM family uses @sc{ieee} floating-point numbers.
+
 @node ARM Directives
 @section ARM Machine Directives

diff --git c/gas/doc/c-cr16.texi i/gas/doc/c-cr16.texi
index b6cf10f..00ddae2 100644
--- c/gas/doc/c-cr16.texi
+++ i/gas/doc/c-cr16.texi
@@ -43,26 +43,33 @@ Operand expression type qualifier is an optional field in the instruction operan
 CR16 target operand qualifiers and its size (in bits):

 @table @samp
-@item Immediate Operand
-- s ---- 4 bits
-@item 
-- m ---- 16 bits, for movb and movw instructions.
-@item 
-- m ---- 20 bits, movd instructions.
-@item 
-- l ---- 32 bits
-
-@item Absolute Operand
-- s ---- Illegal specifier for this operand.
-@item  
-- m ---- 20 bits, movd instructions.
-
-@item Displacement Operand
-- s ---- 8 bits
-@item
-- m ---- 16 bits
-@item 
-- l ---- 24 bits
+@item Immediate Operand: s
+4 bits.
+
+@item Immediate Operand: m
+16 bits, for movb and movw instructions.
+
+@item Immediate Operand: m
+20 bits, movd instructions.
+
+@item Immediate Operand: l
+32 bits.
+
+@item Absolute Operand: s
+Illegal specifier for this operand.
+
+@item Absolute Operand: m
+20 bits, movd instructions.
+
+@item Displacement Operand: s
+8 bits.
+
+@item Displacement Operand: m
+16 bits.
+
+@item Displacement Operand: l
+24 bits
+
 @end table

 For example:
diff --git c/gas/doc/c-mips.texi i/gas/doc/c-mips.texi
index 715091e..1250c1e 100644
--- c/gas/doc/c-mips.texi
+++ i/gas/doc/c-mips.texi
@@ -220,7 +220,7 @@ the @samp{mad} and @samp{madu} instruction, and to not schedule @samp{nop}
 instructions around accesses to the @samp{HI} and @samp{LO} registers.
 @samp{-no-m4650} turns off this option.

-@itemx -m3900
+@item -m3900
 @itemx -no-m3900
 @itemx -m4100
 @itemx -no-m4100
diff --git c/gas/doc/c-score.texi i/gas/doc/c-score.texi
index 0820115..a5b570f 100644
--- c/gas/doc/c-score.texi
+++ i/gas/doc/c-score.texi
@@ -36,7 +36,7 @@ implicitly with the @code{gp} register. The default value is 8.
 @item -EB
 Assemble code for a big-endian cpu

-@itemx -EL
+@item -EL
 Assemble code for a little-endian cpu

 @item -FIXDD
@@ -48,13 +48,13 @@ Assemble code for no warning message for fix data dependency
 @item -SCORE5
 Assemble code for target is SCORE5

-@itemx -SCORE5U
+@item -SCORE5U
 Assemble code for target is SCORE5U

-@itemx -SCORE7
+@item -SCORE7
 Assemble code for target is SCORE7, this is default setting

-@itemx -SCORE3
+@item -SCORE3
 Assemble code for target is SCORE3

 @item -march=score7
diff --git c/gas/doc/c-tic54x.texi i/gas/doc/c-tic54x.texi
index 4cfb440..9d631a6 100644
--- c/gas/doc/c-tic54x.texi
+++ i/gas/doc/c-tic54x.texi
@@ -108,7 +108,7 @@ In this example, x is replaced with SYM2; SYM2 is replaced with SYM1, and SYM1
 is replaced with x.  At this point, x has already been encountered
 and the substitution stops.

-@smallexample @code
+@smallexample
  .asg   "x",SYM1 
  .asg   "SYM1",SYM2
  .asg   "SYM2",x
@@ -125,14 +125,14 @@ Substitution may be forced in situations where replacement might be
 ambiguous by placing colons on either side of the subsym.  The following
 code:

-@smallexample @code
+@smallexample
  .eval  "10",x
 LAB:X:  add     #x, a
 @end smallexample

 When assembled becomes:

-@smallexample @code
+@smallexample
 LAB10  add     #10, a
 @end smallexample

@@ -308,7 +308,7 @@ The @code{LDX} pseudo-op is provided for loading the extended addressing bits
 of a label or address.  For example, if an address @code{_label} resides
 in extended program memory, the value of @code{_label} may be loaded as
 follows:
-@smallexample @code
+@smallexample
  ldx     #_label,16,a    ; loads extended bits of _label
  or      #_label,a       ; loads lower 16 bits of _label
  bacc    a               ; full address is in accumulator A
@@ -344,7 +344,7 @@ Assign @var{name} the string @var{string}.  String replacement is
 performed on @var{string} before assignment.

 @cindex @code{eval} directive, TIC54X
-@itemx .eval @var{string}, @var{name}
+@item .eval @var{string}, @var{name}
 Evaluate the contents of string @var{string} and assign the result as a
 string to the subsym @var{name}.  String replacement is performed on
 @var{string} before assignment.
EOF
cd ..
# 2) Incorporating the changes contained in the patch delievered in mspgcc-20120911
cd gcc-4.7.4
patch --force -p1<../mspgcc-20120911/msp430-gcc-4.7.0-20120911.patch || echo "FIXME: ignoring failure on this patch"
patch --force -p1<../0001-SF-352-Bad-code-generated-pushing-a20-from-stack.patch
patch --force -p1<../0001-SF-357-Shift-operations-may-produce-incorrect-result.patch
patch -p1 <<EOF
diff -u -r gcc-4.7.2-orig/gcc/doc/gcc.texi gcc-4.7.4/gcc/doc/gcc.texi
--- gcc-4.7.2-orig/gcc/doc/gcc.texi	2022-09-26 21:38:56.980428153 +0100
+++ gcc-4.7.4/gcc/doc/gcc.texi	2022-09-26 21:47:14.125105710 +0100
@@ -86,9 +86,15 @@
 @item GNU Press
 @tab Website: www.gnupress.org
 @item a division of the
-@tab General: @tex press@@gnu.org @end tex
+@tab General:
+@tex
+press@@gnu.org
+@end tex
 @item Free Software Foundation
-@tab Orders:  @tex sales@@gnu.org @end tex
+@tab Orders:
+@tex
+sales@@gnu.org
+@end tex
 @item 51 Franklin Street, Fifth Floor
 @tab Tel 617-542-5942
 @item Boston, MA 02110-1301 USA
diff -u -r gcc-4.7.2-orig/gcc/doc/sourcebuild.texi gcc-4.7.4/gcc/doc/sourcebuild.texi
--- gcc-4.7.2-orig/gcc/doc/sourcebuild.texi	2022-09-26 21:38:56.904427412 +0100
+++ gcc-4.7.4/gcc/doc/sourcebuild.texi	2022-09-26 21:47:12.925094362 +0100
@@ -676,7 +676,7 @@
 @code{lang_checks}.

 @table @code
-@itemx all.cross
+@item all.cross
 @itemx start.encap
 @itemx rest.encap
 FIXME: exactly what goes in each of these targets?

EOF
cd ..

# 3) Creating new directories
mkdir binutils-2.22-msp430
mkdir gcc-4.7.4-msp430

# 4) installing binutils in INSTALL_PREFIX
cd binutils-2.22-msp430/
../binutils-2.22/configure --target=msp430 --program-prefix="msp430-" --prefix=$INSTALL_PREFIX
# FIXME: override of CFLAGS to ignore warnings
make CFLAGS="-Wno-error"
make install

# 5) Download the prerequisites
cd ../gcc-4.7.4
./contrib/download_prerequisites
# Build fails if flex is installed on a modern Linux/macOS.
# https://gmplib.org/list-archives/gmp-bugs/2008-August/001114.html
perl -pi -e 's#M4=m4-not-needed#M4=m4#g' gmp-4.3.2/configure

# 6) compiling gcc-4.7.4 in INSTALL_PREFIX
cd ../gcc-4.7.4-msp430
../gcc-4.7.4/configure --target=msp430 --enable-languages=c --program-prefix="msp430-" --prefix=$INSTALL_PREFIX
# FIXME: override of CFLAGS needed to remove -O2, which causes a segfault during compilation of _negdi2
make -j20 CFLAGS="-g"
make install

# 7) compiling msp430mcu in INSTALL_PREFIX
cd ../msp430mcu-20120716
MSP430MCU_ROOT=`pwd` ./scripts/install.sh ${INSTALL_PREFIX}/

# 8) compiling the msp430 lib in INSTALL_PREFIX
cd ../msp430-libc-20120716
cd src
PATH=${INSTALL_PREFIX}/bin:$PATH
make
make PREFIX=$INSTALL_PREFIX install
