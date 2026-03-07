# Set up the variances between the different build modes

CPP = $(CC) -E
LD  := $(LD) -shared
MDFLAGS += -D__GNUC__
# -funsigned-bitfields not supported by clang (Darwin/macOS)
ifneq ($(PLATFORM),darwin)
CFLAGS += -funsigned-bitfields
endif
CFLAGS += -Wcast-align
# Disable glibc _FORTIFY_SOURCE (auto-enabled by GCC 13.3+ / Ubuntu 24.04)
# which causes buffer-overflow aborts for legacy path buffers < PATH_MAX.
CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

BUILD:=$(strip $(BUILD))

ifeq ($(BUILD),PROFILE)
  CFLAGS	+= -O2 -pg
  LDFLAGS	+= -L$(STATIC_LIB) -static
  LIB_EXT	:= a
else
  # Darwin/macOS uses @executable_path for portable installs
  ifeq ($(PLATFORM),darwin)
    # Libs live at $(PREFIX)/bin/lib/ — a child of bin/, not a sibling.
    # @executable_path/lib resolves correctly for any PREFIX layout.
    # Also add $(LIB) absolute rpath for build-time library discovery.
    LDFLAGS	+= -L$(LIB) -Wl,-rpath,@executable_path/lib -Wl,-rpath,$(LIB) $(foreach DIR, $(EXTRA_LD_LIBRARY_PATH), -L$(DIR))
  else
    LDFLAGS	+= -L$(LIB) -Xlinker -R$(LIB) $(foreach DIR, $(EXTRA_LD_LIBRARY_PATH), -Xlinker -R$(DIR))
  endif
  LIB_EXT	:= so
endif

ifeq ($(BUILD),RELEASE)
  CFLAGS	+= -O2
endif

ifeq ($(BUILD),DEVEL)
  DEBUGMODE	= 1
  CFLAGS	+= -g -Wall -O $(DEBUG_CFLAGS)
  YFLAGS	+= -t
endif

ifeq ($(BUILD),DEBUG)
  CFLAGS	+= -g -Wall $(DEBUG_CFLAGS)
  YFLAGS	+= -t
endif

# Define some implicit (pattern) rules

%.a:
	$(AR) -ru $@ $?
	$(RANLIB) $@

%.so:
	$(LD) $(LDFLAGS) $^ -o $@

%.CPP: %.c
	$(CPP) $(CFLAGS) $(CPPFLAGS) $< > $@
