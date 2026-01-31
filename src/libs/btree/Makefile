include $(SRC)/vars.mk

BTREE_OBJ :=    btree.obj       bt_open.obj     bt_look.obj     \
                bt_ins.obj      bt_rem.obj                      \
                                                                \
                btnode.obj      palist.obj      btreec.obj      \
                                                                \
                                                                \
                blkio.obj       bbuf.obj        blkiobuf.obj    \
                dllc.obj
#               bt_lookr.obj
#		share.obj

TRACK_OBJ :=    dbase.obj       dbasec.obj                      \
                track.obj       trackc.obj	dllc.obj

TRACK_OBJ := $(TRACK_OBJ:.obj=.o)
BTREE_OBJ := $(BTREE_OBJ:.obj=.o)

EXTRA_LOADLIBES = -lmaxbtree -lmaxbt -lmsgapi -lstdc++
#LOADLIBES := -lmaxbtree -lmaxbt $(LOADLIBES)

.PHONY:		all tracklib install install_libs install_bin

all:		tracklib trackexp trackimp bttest #audit

install_bin:	trackexp trackimp bttest
		@[ -d "$(BIN)" ] || mkdir -p "$(BIN)"
		cp -f $^ "$(BIN)"

install_libs:	libmaxbtree.so libmaxbt.so
		@[ -d "$(LIB)" ] || mkdir -p "$(LIB)"
		rm -f libmaxdb.so libmaxdb.dylib
		rm -f "$(LIB)/libmaxdb.so" "$(LIB)/libmaxdb.dylib"
		cp -f $^ "$(LIB)"

install:	install_libs install_bin

tracklib:	libmaxbt.so libmaxbtree.so

libmaxbtree.so:	$(TRACK_OBJ) libmaxbt.so
ifeq ($(PLATFORM),darwin)
		$(CXX) -shared -install_name @rpath/$@ $(TRACK_OBJ) $(LDFLAGS) -lmaxbt -lcompat -o $@
else
		$(CXX) -shared $(TRACK_OBJ) $(LDFLAGS) -lmaxbt -lcompat -o $@
endif

libmaxbt.so:	$(BTREE_OBJ)
ifeq ($(PLATFORM),darwin)
		$(CXX) -shared -install_name @rpath/$@ $^ $(LDFLAGS) -lcompat -o $@
else
		$(CXX) -shared $^ $(LDFLAGS) -lcompat -o $@
endif

audit:		audit.o cppmain.o

ctbase:		ctbase.o

clean:
	-rm *.so *.o trackexp trackimp bttest
