# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for maxcfg - Maximus Configuration Editor
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja

SRC ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
NO_DEPEND_RULE := TRUE
include $(SRC)/vars.mk

CFLAGS += -Wall -Wextra -g -I./include $(INCLUDES)
CFLAGS += -I$(SRC)/src/libs/libmaxdb/include -I$(SRC)/src/libs/sqlite

# Enable wide-character ncurses APIs for Unicode rendering.
CFLAGS += -DHAVE_WIDE_CURSES

# Detect macOS vs Linux for ncurses path
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -I/opt/homebrew/opt/ncurses/include
    LDFLAGS += -L/opt/homebrew/opt/ncurses/lib
endif
LDFLAGS += -lncursesw
LDFLAGS += -lmaxcfg
LDFLAGS += -L$(SRC)/src/libs/libmaxdb -lmaxdb -L$(SRC)/src/libs/sqlite -lsqlite3
LDFLAGS += -lmax
LDFLAGS += -lcompat

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
          $(SRCDIR)/ui/picker_with_help.c \
          $(SRCDIR)/ui/modifier_picker.c \
          $(SRCDIR)/ui/command_picker.c \
          $(SRCDIR)/ui/privilege_picker.c \
          $(SRCDIR)/ui/menu_edit.c \
          $(SRCDIR)/ui/menu_preview.c \
          $(SRCDIR)/ui/mci_preview.c \
          $(SRCDIR)/ui/treeview.c \
          $(SRCDIR)/ui/treenode_edit.c \
          $(SRCDIR)/ui/texteditor.c \
          $(SRCDIR)/ui/text_list_editor.c \
          $(SRCDIR)/config/fields.c \
          $(SRCDIR)/config/ctl_parse.c \
          $(SRCDIR)/config/ctl_to_ng.c \
          $(SRCDIR)/config/area_parse.c \
          $(SRCDIR)/config/area_toml.c \
          $(SRCDIR)/config/nextgen_export.c \
          $(SRCDIR)/config/menu_parse.c \
          $(SRCDIR)/config/lang_convert.c \
          $(SRCDIR)/config/lang_browse.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET = maxcfg


.PHONY: all clean dirs install install_binaries install_libs depend

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJDIR)/ui $(OBJDIR)/config

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	@mkdir -p $(BIN)
	cp $(TARGET) $(BIN)/
ifeq ($(PLATFORM),darwin)
	codesign --force --sign - $(BIN)/$(TARGET)
endif

install_binaries: install

install_libs:
	@true

depend:
	@true

# Dependencies
$(OBJDIR)/main.o: $(SRCDIR)/main.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/screen.o: $(SRCDIR)/ui/screen.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/menubar.o: $(SRCDIR)/ui/menubar.c include/maxcfg.h include/ui.h include/menu_data.h include/menu_edit.h include/menu_preview.h include/treeview.h include/nextgen_export.h
$(OBJDIR)/ui/dropdown.o: $(SRCDIR)/ui/dropdown.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/dialog.o: $(SRCDIR)/ui/dialog.c include/maxcfg.h include/ui.h

$(OBJDIR)/ui/form.o: $(SRCDIR)/ui/form.c include/maxcfg.h include/ui.h include/fields.h
$(OBJDIR)/ui/colorpicker.o: $(SRCDIR)/ui/colorpicker.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/colorform.o: $(SRCDIR)/ui/colorform.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/filepicker.o: $(SRCDIR)/ui/filepicker.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/listpicker.o: $(SRCDIR)/ui/listpicker.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/checkpicker.o: $(SRCDIR)/ui/checkpicker.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/picker_with_help.o: $(SRCDIR)/ui/picker_with_help.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/modifier_picker.o: $(SRCDIR)/ui/modifier_picker.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/command_picker.o: $(SRCDIR)/ui/command_picker.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/privilege_picker.o: $(SRCDIR)/ui/privilege_picker.c include/maxcfg.h include/ui.h
$(OBJDIR)/ui/menu_edit.o: $(SRCDIR)/ui/menu_edit.c include/maxcfg.h include/ui.h include/menu_data.h include/menu_edit.h
$(OBJDIR)/ui/mci_preview.o: $(SRCDIR)/ui/mci_preview.c include/mci_preview.h
$(OBJDIR)/ui/menu_preview.o: $(SRCDIR)/ui/menu_preview.c include/maxcfg.h include/ui.h include/menu_preview.h include/menu_data.h include/mci_preview.h
$(OBJDIR)/ui/treeview.o: $(SRCDIR)/ui/treeview.c include/maxcfg.h include/ui.h include/treeview.h
$(OBJDIR)/ui/treenode_edit.o: $(SRCDIR)/ui/treenode_edit.c include/maxcfg.h include/ui.h include/treeview.h
$(OBJDIR)/ui/texteditor.o: $(SRCDIR)/ui/texteditor.c include/maxcfg.h include/ui.h include/texteditor.h
$(OBJDIR)/ui/text_list_editor.o: $(SRCDIR)/ui/text_list_editor.c include/maxcfg.h include/ui.h include/texteditor.h

$(OBJDIR)/config/fields.o: $(SRCDIR)/config/fields.c include/maxcfg.h include/fields.h
$(OBJDIR)/config/ctl_parse.o: $(SRCDIR)/config/ctl_parse.c include/ctl_parse.h
$(OBJDIR)/config/ctl_to_ng.o: $(SRCDIR)/config/ctl_to_ng.c include/ctl_to_ng.h include/menu_data.h
$(OBJDIR)/config/area_parse.o: $(SRCDIR)/config/area_parse.c include/area_parse.h
$(OBJDIR)/config/area_toml.o: $(SRCDIR)/config/area_toml.c include/area_toml.h
$(OBJDIR)/config/nextgen_export.o: $(SRCDIR)/config/nextgen_export.c include/nextgen_export.h include/menu_data.h include/ctl_to_ng.h
$(OBJDIR)/config/menu_parse.o: $(SRCDIR)/config/menu_parse.c include/menu_data.h include/ctl_parse.h
$(OBJDIR)/config/lang_convert.o: $(SRCDIR)/config/lang_convert.c include/lang_convert.h
$(OBJDIR)/config/lang_browse.o: $(SRCDIR)/config/lang_browse.c include/lang_browse.h include/maxcfg.h include/ui.h include/menu_preview.h include/mci_preview.h
