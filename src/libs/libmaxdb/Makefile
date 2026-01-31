# Makefile for libmaxdb
# SPDX-License-Identifier: GPL-2.0-or-later

SRC ?= $(shell pwd)/../../..

include $(SRC)/vars.mk

CFLAGS += -I./include -I$(SRC)/src/libs/slib -I$(SRC)/src/libs/sqlite

OBJS = src/db_init.o src/db_user.o

TARGET = libmaxdb.a

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $(OBJS)

src/db_init.o: src/db_init.c src/db_internal.h include/libmaxdb.h
	$(CC) $(CFLAGS) -c -o $@ src/db_init.c

src/db_user.o: src/db_user.c src/db_internal.h include/libmaxdb.h
	$(CC) $(CFLAGS) -c -o $@ src/db_user.c

clean:
	rm -f $(OBJS) $(TARGET)

install_libs: $(TARGET)
	@[ -d "$(LIB)" ] || mkdir -p "$(LIB)"
	cp -f $^ "$(LIB)"

install: install_libs

.PHONY: all clean install_libs install
