include $(SRC)/vars.mk


#CFLAGS += -DTELNET
.PHONY: all install install_libs

all: libcomm.so

install_libs: libcomm.so
	@[ -d "$(LIB)" ] || mkdir -p "$(LIB)"
	cp -f $^ "$(LIB)"

install: install_libs

libcomm.so: ipcomm.o common.o modemcom.o wincomm.o
ifeq ($(PLATFORM),darwin)
	$(CC) $(CFLAGS) -ggdb -shared -install_name @rpath/$@ -undefined dynamic_lookup $^ -lutil $(LDFLAGS) -lmax -lcompat -o $@
else
	$(CC) $(CFLAGS) -ggdb -shared $^ -lutil $(LDFLAGS) -lmax -lcompat -o $@
endif


clean:
	-rm *.o libcomm.so
