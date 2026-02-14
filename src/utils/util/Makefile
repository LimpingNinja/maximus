include $(SRC)/vars.mk

# maid removed from build — TOML language files replace MAID preprocessing.
PROGS := 	mecca accem ansi2bbs scanbld ansi2mec cvtusr \
		editcall mr fixlr setlr fb maxcomm init_userdb import_userdb #oracle maxpipe mxpipe32 piper

all: $(PROGS)
install: $(PROGS)
#install: maid mecca accem fb ansi2bbs
	cp -f $^ $(BIN)

mecca: mecca.o init.o
oracle: EXTRA_INCLUDES += -I$(SRC)/resources/lang

mecca.c init.c: mecca_vb.h

mecca_vb.h: mecca_vb.h.in
	sort < $< > $@

mecca.h: mecca_vb.h
mecca.o init.o accem.o: mecca.h
maxcomm fixlr setlr fb mr editcall mecca cvtusr ansi2mec ansi2bbs oracle: EXTRA_LOADLIBES += -lmsgapi
fb mr: EXTRA_LOADLIBES += -lmaxcfg

init_userdb: init_userdb.o $(SRC)/src/libs/sqlite/libsqlite3.a
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LOADLIBES) -o $@

import_userdb: EXTRA_INCLUDES += -I$(SRC)/src/libs/libmaxdb/include
import_userdb: EXTRA_LOADLIBES += -lmaxcfg
import_userdb: import_userdb.o $(SRC)/src/libs/libmaxdb/libmaxdb.a $(SRC)/src/libs/sqlite/libsqlite3.a
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LOADLIBES) -o $@

ansi2bbs.o: ansi2bbs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DANSI2BBS -o $@ -c $<

ansi2mec.o: ansi2bbs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DANSI2MEC -o $@ -c $<

# Wes: strange build line forces g++ for linking with objects containing cppmain.o.
# This is needed for gcc 3; otherwise we'd need to figure out which targets to
# add libgcc_s.so to, and only only gcc 3. This should work for gcc 2 and gcc 3. 

cvtusr scanbld maxcomm: 
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ $(LOADLIBES) -o $@

cvtusr:		cvtusr.o cppmain.o cvt_misc.o
scanbld accem: EXTRA_LOADLIBES += -lmsgapi
scanbld: EXTRA_LOADLIBES += -lmaxcfg
accem:		accem.o init.o
scanbld:	cppmain.o scanbld.o
maxcomm:	maxcomm.o
oracle:		oracle.o


clean:
	rm *.o $(PROGS)

max2priv.o:	CPPFLAGS += -I$(SRC)/resources/lang
