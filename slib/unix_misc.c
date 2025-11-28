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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#if defined(__APPLE__) || defined(DARWIN)
# include <util.h>
#else
# include <pty.h>
# include <utmp.h>
#endif
#include "process.h"
#include "io.h"
#include "prog.h"
#include "ntcomm.h"

#define unixfd(hc)      FileHandle_fromCommHandle(ComGetHandle(hc))

/* Record locking code */

int lock(int fd, long offset, long len)
{
  struct flock  lck;

  lck.l_type    = F_WRLCK;                      /* setting a write lock */
  lck.l_whence  = SEEK_SET;                     /* offset l_start from beginning of file */
  lck.l_start   = (off_t)offset;
  lck.l_len     = len;

  return fcntl(fd, F_SETLK, &lck);
}

int unlock(int fd, long offset, long len)
{
  struct flock  lck;

  lck.l_type    = F_UNLCK;                      /* setting not locked */
  lck.l_whence  = SEEK_SET;                     /* offset l_start from beginning of file */
  lck.l_start   = (off_t)offset;
  lck.l_len     = len;

  return fcntl(fd, F_SETLK, &lck);
}

void NoMem() __attribute__((weak));
void NoMem()
{
  fprintf(stderr, "Out of memory!\n");
  _exit(1);
}
