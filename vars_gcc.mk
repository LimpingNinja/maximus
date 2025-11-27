# Set up the variances between the different build modes

CPP = $(CC) -E
LD  := $(LD) -shared
MDFLAGS += -D__GNUC__
# -funsigned-bitfields not supported by clang (Darwin/macOS)
ifneq ($(PLATFORM),darwin)
CFLAGS += -funsigned-bitfields
endif
CFLAGS += -Wcast-align

BUILD:=$(strip $(BUILD))

ifeq ($(BUILD),PROFILE)
  CFLAGS	+= -O2 -pg
  LDFLAGS	+= -L$(STATIC_LIB) -static
  LIB_EXT	:= a
else
  # Darwin/macOS uses @executable_path for portable installs
  ifeq ($(PLATFORM),darwin)
    # Use @executable_path/../lib so binaries find libs relative to their location
    # This works for any PREFIX since bin/ and lib/ are always siblings
    # Also add $(LIB) rpath for build-time library discovery
    LDFLAGS	+= -L$(LIB) -Wl,-rpath,@executable_path/../lib -Wl,-rpath,$(LIB) $(foreach DIR, $(EXTRA_LD_LIBRARY_PATH), -L$(DIR))
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
