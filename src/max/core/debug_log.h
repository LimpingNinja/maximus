/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 */

#ifndef __DEBUG_LOG_H_DEFINED
#define __DEBUG_LOG_H_DEFINED

void debug_log_open(void);
void debug_log(const char *fmt, ...);
void debug_log_close(void);

#endif
