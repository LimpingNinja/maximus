#ifndef _WES_COMPAT_H
# define _WES_COMPAT_H
# include "winstr.h"
# include "dossem.h"
# include "dosproc.h"
# include "viocurses.h"
# include "io.h"
# define _export

/* Case adaptation for Unix filesystems (from adcase.c) */
void adaptcase(char *pathname);
void adaptcase_refresh_dir(const char *directory);

#endif
