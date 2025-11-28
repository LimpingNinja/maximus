/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
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

#ifndef _WES_IO_H
#define _WES_IO_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>

#ifndef O_BINARY
# define O_BINARY 0
# define O_TEXT 0
#endif

#ifdef LINUX
# define O_NOINHERIT 01000000
#endif

#ifdef SOLARIS
# define O_NOINHERIT 0x10000
#endif

#ifdef __FreeBSD__
# define O_NOINHERIT 0x20000
#endif

#if defined(__APPLE__) || defined(DARWIN)
# include <fcntl.h>
# ifdef O_CLOEXEC
#  define O_NOINHERIT O_CLOEXEC
# else
#  define O_NOINHERIT 0x1000000
# endif
#endif

#ifndef O_NOINHERIT
# error You must choose a value for O_NOINHERIT which does not conflict with other vendor open flags!
#endif
 
int sopen(const char *filename, int openMode, int shacc, ...);
char *fixPathDup(const char *filename);
void fixPathDupFree(const char *filename, char *filename_dup);
char *fixPath(char *filename);
void fixPathMove(char *filename);
int sopen(const char *filename, int openMode, int shacc, ...);
long tell(int fd);
int fputchar(int c);
#endif
