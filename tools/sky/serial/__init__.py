#!/usr/bin/env python3
#portable serial port access with python
#this is a wrapper module for different platform implementations
#
# (C)2001-2002 Chris Liechti <cliechti@gmx.net>
# this is distributed under a free software license, see license.txt

import os
VERSION = "$Revision: 1.1 $".split()[1]     #extract CVS version

#chose an implementation, depending on os
if os.name == 'nt': #sys.platform == 'win32':
    from .serialwin32 import *
elif os.name == 'posix':
    from .serialposix import *
elif os.name == 'java':
    from .serialjava import *
else:
    raise "Sorry no implementation for your platform available."

# no "mac" implementation. someone wants to write it? I have no access to a mac.
