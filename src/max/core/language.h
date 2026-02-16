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

#ifndef __LANGUAGE_H_DEFINED
#define __LANGUAGE_H_DEFINED


#ifdef MAX_INCL_LANGUAGE

/*
 * Language system — all string retrieval is now TOML-based via the maxlang
 * API.  The legacy .ltf binary heap format has been removed.
 *
 * english.h provides backward-compatible C macros that expand to
 * maxlang_get() calls.  s_ret() and s_reth() are retained for legacy
 * MEX intrinsic backward compatibility (lstr/hstr).
 */

#ifdef MAX_INCL_LANGLTH
  #include "english.h"
#endif

cpp_begin()
  char *s_ret(word strn);
cpp_end()

#endif /* MAX_INCL_LANGUAGE */

#endif /* __LANGUAGE_H_DEFINED */

