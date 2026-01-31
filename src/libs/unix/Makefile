include $(SRC)/vars.mk

.PHONY: all install install_libs

all: libcompat.so

install_libs: libcompat.so
	@[ -d "$(LIB)" ] || mkdir -p "$(LIB)"
	cp -f $^ "$(LIB)"

install: install_libs

libcompat.so: adcase.o dosio.o winstr.o conio.o viocurses.o dossem.o dosproc.o xpunix.o process.o
ifeq ($(PLATFORM),darwin)
	$(CC) $(CFLAGS) -ggdb -shared -install_name @rpath/$@ $^ -lutil -lncurses $(LDFLAGS) -o $@
else
	$(CC) $(CFLAGS) -ggdb -shared $^ -lutil -lncurses $(LDFLAGS) -o $@
endif


clean:
	-rm *.o libcompat.so
