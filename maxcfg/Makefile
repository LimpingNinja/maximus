# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for maxcfg - Maximus Configuration Editor
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

SRC = ..
include $(SRC)/vars.mk

# Use same includes as Maximus for prm.h compatibility
CFLAGS += -Wall -Wextra -g -I./include $(INCLUDES)

# Detect macOS vs Linux for ncurses path
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -I/opt/homebrew/opt/ncurses/include
    LDFLAGS += -L/opt/homebrew/opt/ncurses/lib
endif
LDFLAGS += -lncurses

SRCDIR = src
OBJDIR = obj

SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/ui/screen.c \
          $(SRCDIR)/ui/menubar.c \
          $(SRCDIR)/ui/dropdown.c \
          $(SRCDIR)/ui/dialog.c \
          $(SRCDIR)/ui/form.c \
          $(SRCDIR)/ui/colorpicker.c \
          $(SRCDIR)/ui/colorform.c \
          $(SRCDIR)/ui/filepicker.c \
          $(SRCDIR)/ui/listpicker.c \
          $(SRCDIR)/ui/checkpicker.c \
          $(SRCDIR)/ui/treeview.c \
          $(SRCDIR)/config/fields.c \
          $(SRCDIR)/data/prm.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET = maxcfg

.PHONY: all clean dirs install

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJDIR)/ui $(OBJDIR)/config $(OBJDIR)/data

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	cp $(TARGET) $(BIN)/
ifeq ($(PLATFORM),darwin)
	codesign --force --sign - $(BIN)/$(TARGET)
endif

# Dependencies
$(OBJDIR)/main.o: $(SRCDIR)/main.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/screen.o: $(SRCDIR)/ui/screen.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/menubar.o: $(SRCDIR)/ui/menubar.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/dropdown.o: $(SRCDIR)/ui/dropdown.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/dialog.o: $(SRCDIR)/ui/dialog.c include/maxcfg.h include/ui.h
