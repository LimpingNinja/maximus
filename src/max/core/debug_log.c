/*
 * debug_log.c — Debug logging implementation
 *
 * Copyright 2026 by Kevin Morgan.  All rights reserved.
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
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

static FILE *debug_fp = NULL;

/**
 * @brief Open the debug log file for appending.
 *
 * Opens "debug.log" in append mode with line-buffered output.
 * Safe to call multiple times; subsequent calls are no-ops if
 * the file is already open.
 */
void debug_log_open(void)
{
  if (!debug_fp)
  {
    debug_fp = fopen("debug.log", "a");
    if (debug_fp)
    {
      setvbuf(debug_fp, NULL, _IOLBF, 0);
    }
  }
}

/**
 * @brief Write a timestamped debug message to the log file.
 *
 * Automatically opens the log if not already open. Each line is
 * prefixed with an ISO-style timestamp and the current process ID.
 *
 * @param fmt  printf-style format string
 * @param ...  Format arguments
 */
void debug_log(const char *fmt, ...)
{
  va_list args;
  time_t now;
  struct tm *tm_info;
  char time_buf[64];
  
  if (!debug_fp)
    debug_log_open();
    
  if (!debug_fp)
    return;
    
  time(&now);
  tm_info = localtime(&now);
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
  
  fprintf(debug_fp, "[%s pid=%ld] ", time_buf, (long)getpid());
  
  va_start(args, fmt);
  vfprintf(debug_fp, fmt, args);
  va_end(args);
  
  fprintf(debug_fp, "\n");
  fflush(debug_fp);
}

/**
 * @brief Close the debug log file.
 *
 * Flushes and closes the debug log. Safe to call even if the log
 * was never opened.
 */
void debug_log_close(void)
{
  if (debug_fp)
  {
    fclose(debug_fp);
    debug_fp = NULL;
  }
}
