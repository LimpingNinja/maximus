include $(SRC)/vars.mk

EXTRA_CPPFLAGS += -DHEAP_SIGNATURE

VMDLL   := libmexvm.so

VMALL_OBJS :=   vm_run.obj      vm_heap.obj     vm_symt.obj             \
               vm_read.obj     vm_opcvt.obj    vm_opflo.obj            \
               vm_opfun.obj    vm_opmth.obj    vm_opstk.obj            \
               vm_opstr.obj

VMALL_OBJS := $(VMALL_OBJS:.obj=.o)

CPPFLAGS += -DWES_HACK

.PHONY: all install install_libs clean

all: $(VMDLL)

$(VMDLL): $(VMALL_OBJS)
ifeq ($(PLATFORM),darwin)
	$(CC) -shared -install_name @rpath/$@ -undefined dynamic_lookup $^ $(LDFLAGS) -lmax -lcompat -o $@
else
	$(CC) -shared $^ $(LDFLAGS) -lmax -lcompat -o $@
endif

vm_dll.obj: vm_dll.c
	$(CC) $(CFLAGS) $(C_DLL) vm_dll

install_libs: $(VMDLL)
	cp -f $^ $(LIB)

install: install_libs

clean:
	-rm *.o *.a *.so
