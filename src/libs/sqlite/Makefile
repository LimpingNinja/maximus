include $(SRC)/vars.mk

.PHONY: all install_libs install clean

all: libsqlite3.a

libsqlite3.a: sqlite3.o
	$(AR) rcs $@ $^
	$(RANLIB) $@

sqlite3.o: sqlite3.c sqlite3.h sqlite3ext.h
	$(CC) $(CFLAGS) -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_RTREE -DSQLITE_ENABLE_JSON1 -c sqlite3.c -o $@

install_libs: libsqlite3.a
	@[ -d "$(LIB)" ] || mkdir -p "$(LIB)"
	cp -f $^ "$(LIB)"

install: install_libs

clean:
	-rm -f *.o *.a
