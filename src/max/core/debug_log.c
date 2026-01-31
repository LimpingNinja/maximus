/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

static FILE *debug_fp = NULL;

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

void debug_log_close(void)
{
  if (debug_fp)
  {
    fclose(debug_fp);
    debug_fp = NULL;
  }
}
