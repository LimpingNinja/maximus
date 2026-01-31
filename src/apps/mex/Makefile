include $(SRC)/vars.mk

EXTRA_CPPFLAGS += -DMEX_COMPILER
EXTRA_INCLUDES += -I. -I$(SRC)/src/libs/mexvm

LOADLIBES := $(LOADLIBES) -lmsgapi

TABOBJ  :=      mex_tab.obj

SEMOBJS :=      sem_decl.obj sem_func.obj sem_scop.obj sem_expr.obj       \
               sem_flow.obj sem_goto.obj sem_gen.obj  sem_vm.obj

OBJS    :=      mex_main.obj mex_lex.obj  mex_symt.obj $(SEMOBJS)         \
               mex_misc.obj mex_err.obj

OBJS := $(OBJS:.obj=.o)
TABOBJ := $(TABOBJ:.obj=.o)
SEMOBJS := $(SEMOBJS:.obj=.o)

.PHONY: all install install_binaries clean

all: mex

# Expect 1 shift/reduce conflict
mex_tab.c: mex_tab.y

# Ensure token header exists before compiling sources that include mex.h
$(OBJS): mex_tab.h
$(TABOBJ): mex_tab.h

mex: $(TABOBJ) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LOADLIBES)

install_binaries: mex
	cp -f $^ $(BIN)

install: install_binaries

clean:
	-rm mex_tab.c mex_tab.h *.o mex
