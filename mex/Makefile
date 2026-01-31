include $(SRC)/vars.mk
 
.PHONY: all install install_libs install_binaries clean
 
all:
	$(MAKE) SRC=$(SRC) -C $(SRC)/src/libs/mexvm
	$(MAKE) SRC=$(SRC) -C $(SRC)/src/apps/mex
 
install_libs:
	$(MAKE) SRC=$(SRC) -C $(SRC)/src/libs/mexvm install_libs
 
install_binaries:
	$(MAKE) SRC=$(SRC) -C $(SRC)/src/apps/mex install_binaries
 
install: install_libs install_binaries
 
clean:
	$(MAKE) SRC=$(SRC) -C $(SRC)/src/libs/mexvm clean || true
	$(MAKE) SRC=$(SRC) -C $(SRC)/src/apps/mex clean || true
