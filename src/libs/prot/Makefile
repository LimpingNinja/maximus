include $(SRC)/vars.mk

OBJS = frecv.o   fsend.o   pdata.o   rbsb.o    rz.o      sz.o      zm.o
CPPFLAGS += -I$(SRC)/resources/lang

all: libxfer.so

libxfer.so: $(OBJS)
ifeq ($(PLATFORM),darwin)
	$(CC) -shared -install_name @rpath/$@ -undefined dynamic_lookup $^ $(LDFLAGS) -lmax -lcompat -o $@
else
	$(CC) -shared $^ $(LDFLAGS) -lmax -lcompat -o $@
endif

install_libs install: libxfer.so
	cp libxfer.so $(LIB)

clean:
	-rm $(OBJS) libxfer.so
