/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 *   - Added automatic dropfile generation for door programs
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

#ifndef __DROPFILE_H_DEFINED
#define __DROPFILE_H_DEFINED

/* Dropfile generation functions */
int Write_Dorinfo1(void);
int Write_DoorSys(void);
int Write_ChainTxt(void);
int Write_Door32Sys(void);
int Write_All_Dropfiles(void);

/* Cleanup function */
void Clean_Node_Temp_Dir(void);

#endif /* __DROPFILE_H_DEFINED */
