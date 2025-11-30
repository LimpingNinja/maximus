# Linux with GNU Make, GNU C
#
# Tested on: Ubuntu, Debian, Alpine
# Note: Requires ncurses-dev package
#
# $Id$

EXTRA_CPPFLAGS	+= -DLINUX
# Note: -lmsgapi repeated at end to resolve circular dependency with libmax
OS_LIBS		= -lpthread -lncurses -lmsgapi

# Position-independent code for shared libraries
CFLAGS		+= -fPIC
CXXFLAGS	+= -fPIC

# Linux rpath for finding shared libraries relative to executable
LINUX_RPATH	= -Wl,-rpath,'$$ORIGIN/../lib'
LDFLAGS		+= $(LINUX_RPATH)

# Default prefix if not set
ifeq ($(PREFIX),)
PREFIX		= /var/max
endif
