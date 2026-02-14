# @file 	Makefile	Master Makefile for Makefile 3.03 onward
# @author			Wes Garland
# @date				May 13th, 2003
#
# $Id: Makefile,v 1.9 2004/01/19 23:37:01 paltas Exp $
# $Log: Makefile,v $
# Revision 1.9  2004/01/19 23:37:01  paltas
# Added some to get freebsd work, and improved maxcomm a bit..
#
# Revision 1.8  2003/10/05 01:56:37  rfj
# Updated master Makefile to not build SqaFix when compiling just Maximus
# code.
#
# Revision 1.7  2003/08/15 19:57:52  rfj
# Master makefile updated to support SqaFix source code as part of the Maximus
# SourceForge project.  SqaFix program is now under GPL.
#
# Revision 1.6  2003/06/29 20:38:51  wesgarland
# Cosmetic change
#
# Revision 1.5  2003/06/12 03:26:43  wesgarland
# Corrected PREFIX-passing between master Makefile and copy_install_ree.sh
#
# Revision 1.4  2003/06/12 02:50:52  wesgarland
# Modified to better support non-standard PREFIX
#
# Revision 1.3  2003/06/11 19:23:53  wesgarland
# Successfully performs a "make distclean; ./configure; make build; make install"
#
#

SQUISH_LIB_DIRS = src/libs/btree src/libs/unix src/libs/slib src/libs/msgapi src/utils/squish
SQAFIX_LIB_DIRS = src/libs/msgapi src/utils/sqafix
MAX_LIB_DIRS    = src/libs/unix src/libs/slib src/libs/msgapi src/libs/mexvm src/libs/prot src/libs/legacy/comdll src/libs/libmaxcfg src/libs/sqlite src/libs/libmaxdb 
LIB_DIRS	= $(SQUISH_LIB_DIRS) $(SQAFIX_LIB_DIRS) $(MAX_LIB_DIRS)
PROG_DIRS	= src/utils/squish src/max src/apps/mex src/utils/util src/apps/maxcfg
MAXTEL_DIR	= src/apps/maxtel
DIRS		= $(LIB_DIRS) $(PROG_DIRS) src/utils/sqafix $(MAXTEL_DIR)
NO_DEPEND_RULE	:= TRUE

topmost:: header usage

include vars.mk
MAXIMUS=$(PREFIX)

.PHONY: all depend clean install mkdirs squish max install_libs install_binaries \
	usage topmost build config_install configure reconfig sqafix maxtel maxtel_install \


header::
	@echo "Maximus-CBCS Master Makefile"
	@echo 

usage::
	@echo "Maximus was written by Scott Dudley (Lanius Corporation),"
	@echo "Peter Fitzsimmions, and David Nugent, and released in 2002"
	@echo "under the GPL (GNU Public Licence). The UNIX port is by"
	@echo 'Wes Garland. Type "make gpl" to view the text of the'
	@echo "licence."
	@echo
	@echo "Paths:                  (edit vars.mk to change)"
	@echo "         prefix         $(PREFIX)"
	@echo "         libraries      $(LIB)"
	@echo "         binaries       $(BIN)"
	@echo      
	@echo "Targets:"
	@echo "         build          build maximus, squish and SqaFix"
	@echo "         config_install install configuration files"
	@echo "         install        build and install everything"
	@echo "         squish         build squish"
	@echo "         squish_install build and install squish"
	@echo "         sqafix         build SqaFix"
	@echo "         sqafix_install build and install SqaFix"
	@echo "         max            build maximus"
	@echo "         max_install    build and install maximus"
	@echo "         maxtel         build maxtel supervisor"
	@echo "         maxtel_install build and install maxtel"
	@echo

mkdirs:
	[ -d "$(LIB)" ] || mkdir -p "$(LIB)"
	[ -d "$(BIN)" ] || mkdir -p "$(BIN)"

all:	mkdirs clean squish_install max_install sqafix_install

clean: buildclean
	$(foreach DIR, $(DIRS) configuration-tests, $(MAKE) SRC=$(SRC) -C $(DIR) -k $@; )
	-rm depend.mk.bak depend.mk
	-rm */depend.mk.bak */depend.mk

# buildclean: Clean the build folder for fresh install
buildclean:
	@echo "Cleaning build folder $(PREFIX)..."
	-rm -rf $(PREFIX)/bin $(PREFIX)/lib $(PREFIX)/libexec
	-rm -rf $(PREFIX)/etc $(PREFIX)/m $(PREFIX)/docs
	@echo "Build folder cleaned. Next 'make build' will be fresh."

# archclean: Also clean build/lib for cross-architecture builds
archclean: clean buildclean
	@echo "Architecture-clean complete. Ready for cross-arch build."

# fullclean: Clean everything including build folder
fullclean: clean buildclean
	@echo "Full clean complete."


dist-clean distclean: clean buildclean
	-rm src/libs/slib/compiler_details.h
	-rm vars.mk vars_local.mk

depend:
	@true

install_libs:
	$(foreach DIR, $(LIB_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR) -k $@; )

install_binaries:
	$(foreach DIR, $(PROG_DIRS) src/utils/sqafix $(MAXTEL_DIR), $(MAKE) SRC=$(SRC) -C $(DIR) -k install; )

squish_install: mkdirs
	$(foreach DIR, $(SQUISH_LIB_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR) install_libs; )
	$(MAKE) SRC=$(SRC) -C src/utils/squish install

sqafix_install: mkdirs
	$(foreach DIR, $(SQAFIX_LIB_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR) install_libs; )
	$(MAKE) SRC=$(SRC) -C src/utils/sqafix install

max_install: mkdirs
	$(foreach DIR, $(MAX_LIB_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR) install_libs; )
	$(MAKE) SRC=$(SRC) -C src/utils/util
	$(foreach DIR, $(PROG_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR) install; )
ifeq ($(PLATFORM),darwin)
	@echo "Codesigning binaries for macOS..."
	@for f in $(BIN)/*; do codesign -f -s - "$$f" 2>/dev/null || true; done
	@for f in $(LIB)/*; do codesign -f -s - "$$f" 2>/dev/null || true; done
endif

squish:
	$(foreach DIR, $(SQUISH_LIB_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR); )
	$(MAKE) SRC=$(SRC) -C src/utils/squish

sqafix:
	$(foreach DIR, $(SQAFIX_LIB_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR); )
	$(MAKE) SRC=$(SRC) -C src/utils/sqafix

max:
	$(foreach DIR, $(MAX_LIB_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR); )
	$(MAKE) SRC=$(SRC) -C src/utils/util
	$(foreach DIR, $(PROG_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR); )

maxtel:
	$(foreach DIR, $(MAX_LIB_DIRS), $(MAKE) SRC=$(SRC) -C $(DIR); )
	$(MAKE) SRC=$(SRC) -C $(MAXTEL_DIR)

maxtel_install: mkdirs maxtel
	$(MAKE) SRC=$(SRC) -C $(MAXTEL_DIR) install

configure:
	./configure "--prefix=$(PREFIX)"

config_install:
	@export PREFIX
	@scripts/copy_install_tree.sh "$(PREFIX)"
 
	@$(MAKE) reconfig
 
	@[ -d ${PREFIX}/etc/lang ] || mkdir -p ${PREFIX}/etc/lang
	@cp -f build/etc/lang/english.toml ${PREFIX}/etc/lang/english.toml
	@cp -f resources/lang/delta_english.toml ${PREFIX}/etc/lang/delta_english.toml

	@[ -d ${PREFIX}/etc/db ] || mkdir -p ${PREFIX}/etc/db
	@cp -f scripts/db/userdb_schema.sql ${PREFIX}/etc/db/userdb_schema.sql
	@cp -f scripts/db/init-userdb.sh ${PREFIX}/bin/init-userdb.sh
	@chmod +x ${PREFIX}/bin/init-userdb.sh

	@[ ! -f ${PREFIX}/etc/user.db ] || echo "This is not a fresh install -- not creating new user.db"
	@[ -f ${PREFIX}/etc/user.db ] || echo "Creating user.db"
	@[ -f ${PREFIX}/etc/user.db ] || (cd ${PREFIX} && bin/init-userdb.sh ${PREFIX} || true)
	@echo
	@echo "Configuration complete."

reconfig:
	@echo " - Compiling MECCA help files"
	@(cd $(PREFIX)/etc/help && for f in *.mec; do ../../bin/mecca "$$f" 2>&1 || true; done)

	@echo " - Compiling misc MECCA files"
	@(cd $(PREFIX)/etc/misc && for f in *.mec; do ../../bin/mecca "$$f" 2>&1 || true; done)

	@echo " - Compiling MEX files"
	@(cd $(PREFIX)/m && export MEX_INCLUDE=$(PREFIX)/m && for f in *.mex; do ../bin/mex "$$f" 2>&1 || true; done)

	@echo
	@echo "reconfig complete (MAID removed — language strings now via TOML)"

install: mkdirs squish_install sqafix_install max_install maxtel_install config_install

build:	mkdirs install_libs squish sqafix max maxtel install_binaries
	@echo "Build Complete; edit your control files and 'make install'"

GPL gpl license::
	@[ -x /usr/bin/less ] && cat LICENSE | /usr/bin/less || true
	@[ ! -x /usr/bin/less ] && cat LICENSE | more || true
