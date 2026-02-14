include $(SRC)/vars.mk

.PHONY: all install install_libs clean

CFLAGS += -I./include

OBJS := src/libmaxcfg.o src/path.o src/maxlang.o

all: libmaxcfg.so

libmaxcfg.so: $(OBJS)
ifeq ($(PLATFORM),darwin)
	$(CC) $(CFLAGS) -shared -install_name @rpath/$@ $^ $(LDFLAGS) -o $@
else
	$(CC) $(CFLAGS) -shared $^ $(LDFLAGS) -o $@
endif

install_libs: libmaxcfg.so
	@[ -d "$(LIB)" ] || mkdir -p "$(LIB)"
	cp -f $^ "$(LIB)"

install: install_libs

clean:
	-rm -f $(OBJS) libmaxcfg.so
