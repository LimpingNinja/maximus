/*
 * debug_log.h — Debug logging header
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

#ifndef __DEBUG_LOG_H_DEFINED
#define __DEBUG_LOG_H_DEFINED

/**
 * @brief Open the debug log file for appending.
 */
void debug_log_open(void);

/**
 * @brief Write a timestamped debug message to the log file.
 *
 * @param fmt  printf-style format string
 * @param ...  Format arguments
 */
void debug_log(const char *fmt, ...);

/**
 * @brief Close the debug log file.
 */
void debug_log_close(void);

#endif
