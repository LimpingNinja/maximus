/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*# name=Compiler-determination and memory-model-determination routines
    name=Support for WATCOM C (DOS, OS/2 and 386 flat), Microsoft C
    name=(all memory models, DOS & OS/2), Turbo C 2.0, Turbo/Borland C++
    name=and IBM C Set/2 (flat model).
*/

/* Non-DOS systems...  Just do a "#define __FARCODE__",                     *
 * "#define __FARDATA__" and "#define __LARGE__" in place of this file.     */

#ifndef __COMPILER_UNIX_H_DEFINED
#define __COMPILER_UNIX_H_DEFINED

/* Hacks to build under unix by Wes -- need to patch up later
 * if we want to support other platforms
 */

#ifndef UNIX
# error Wrong header file, or no -DUNIX!
#endif

#ifndef _POSIX_SOURCE
# define _POSIX_SOURCE
#endif

#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "compat.h"

#define __POSIX__
#define __FARCODE__
#define __FARDATA__
#define __LARGE__
#define __FLAT__
#define EXPENTRY

#define _stdc
#define _vstdc
#define _intr
#define _intcast
#define _veccast
#define _fast
#define _loadds

#define pascal
#define cdecl
#define far
#define near
#define huge

#define NW(var) (void)var

/* DLL init/fini - _init/_fini conflict with glibc on modern Linux.
 * Use constructor/destructor attributes instead. */
#if defined(__GNUC__)
# define DLL_CONSTRUCTOR __attribute__((constructor))
# define DLL_DESTRUCTOR  __attribute__((destructor))
#else
# define DLL_CONSTRUCTOR
# define DLL_DESTRUCTOR
#endif
/* Legacy macro - expands to a static constructor function */
# define __dll_initialize DLL_CONSTRUCTOR static void _maximus_dll_init_func
# define __dll_terminate  DLL_DESTRUCTOR static void _maximus_dll_fini_func

#define mkdir(a) mkdir(a, 0777)
#endif /* ! __COMPILER_UNIX_H_DEFINED */
