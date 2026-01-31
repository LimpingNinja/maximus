# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for maxtel - Multi-node Telnet Supervisor
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja

SRC ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
include $(SRC)/vars.mk

TARGET = maxtel
SRCS = maxtel.c
OBJS = $(SRCS:.c=.o)

# Additional flags for maxtel
CPPFLAGS += -DUNIX
ifeq ($(PLATFORM),darwin)
    CPPFLAGS += -DDARWIN
    MAXTEL_LIBS = -lmax -lmaxcfg -lcompat -lncurses
else
    # Note: -lmsgapi needed for SquishHash used by libmax
    MAXTEL_LIBS = -lmax -lmaxcfg -lcompat -lncurses -lutil -lmsgapi
endif

.PHONY: all clean install install_libs

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(MAXTEL_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)

install: $(TARGET)
	cp $(TARGET) $(BIN)/
ifeq ($(PLATFORM),darwin)
	codesign --force --sign - $(BIN)/$(TARGET)
endif

install_libs:
	@# No libraries to install
