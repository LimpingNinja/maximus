# Darwin (macOS) with GNU Make, Clang/GCC
# Based on vars_freebsd.mk since Darwin is BSD-derived
#
# Tested on: macOS (Darwin) 
# Note: macOS uses ncurses from the system or Homebrew
#
# $Id$

# Darwin is BSD-based
# NEED_MEMALIGN: macOS doesn't have memalign(), use fallback implementation
EXTRA_CPPFLAGS	+= -DBSD -DDARWIN -DHAVE_TIMER_T -DNEED_MEMALIGN

# macOS doesn't need -pthread for linking (it's built-in)
# but we still need it for compilation
OS_LIBS		= -lpthread -lncurses

# makedepend flags for Darwin
MDFLAGS		+= -D__APPLE__ -D__DARWIN__ -DBSD

# Architecture override (set ARCH=x86_64 for Rosetta testing)
ifdef ARCH
CFLAGS		+= -arch $(ARCH)
CXXFLAGS	+= -arch $(ARCH)
LDFLAGS		+= -arch $(ARCH)
endif

# Position-independent code for shared libraries
CFLAGS		+= -fPIC
CXXFLAGS	+= -fPIC

# NOTE: rpath and library link flags are set in vars_gcc.mk (the LDFLAGS block).
# DARWIN_RPATH / DARWIN_LDFLAGS were previously defined here but never consumed.
# Removed to avoid confusion — vars_gcc.mk is the single source of truth.

# macOS shared library install name - use @rpath for relocatable libs
# Libraries will be found via the rpath set in binaries
DARWIN_SOFLAGS	= -install_name @rpath/$(@F)

# Silence some common warnings on modern compilers
CFLAGS		+= -Wno-deprecated-declarations -Wno-implicit-function-declaration
CXXFLAGS	+= -Wno-deprecated-declarations

# Clang doesn't support -funsigned-bitfields, remove it
CFLAGS		:= $(filter-out -funsigned-bitfields,$(CFLAGS))

# Default prefix if not set
ifeq ($(PREFIX),)
PREFIX		= /usr/local/max
endif

# macOS specific: shared library extension is .dylib but we'll use .so
# for compatibility with the existing build system
# LIB_EXT = dylib  # Uncomment to use native macOS extension

# -DBSD: Darwin is a BSD variant
# -DDARWIN: Darwin-specific code paths
# -DHAVE_TIMER_T: Darwin defines timer_t in sys/types.h
# MDFLAGS: Preprocessor flags for makedepend

