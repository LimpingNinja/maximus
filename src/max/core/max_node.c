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

#error this file no longer used.


/*# name=Nodelist searching and retrieval functions
*/

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_sysop
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <share.h>
#include "prog.h"
#include "mm.h"

static int near NodeExist(NETADDR *d)
{
  struct _ndi ndi[NUM_NDI];

  int idxfile;
  long offset;
  word current_zone;
  word x, y;

  char temp[PATHLEN];
  int nlver;

  current_zone=ngcfg_get_matrix_primary_address().zone;
  nlver = ngcfg_get_nodelist_version_int();

  if (! *ngcfg_get_string_raw("maximus.net_info_path"))
    return -1;

  LangSprintf(temp, sizeof(temp), idxnode, ngcfg_get_string_raw("maximus.net_info_path"));

  if ((idxfile=shopen(temp,O_RDONLY | O_BINARY))==-1)
  {
/*    cant_open(temp);*/
    return -1;
  }

  for (offset=0L;
       (y=read(idxfile, (char *)ndi, sizeof(*ndi) * NUM_NDI)) > sizeof(*ndi);
       offset += (long)y)
  {
    y /= sizeof(struct _ndi);

    for (x=0; x < y; x++)
    {
      if (ndi[x].node==(word)-2)
        current_zone=ndi[x].net;

      /* Convert region/zone addresses */

      if ((sword)ndi[x].node < 0)
        ndi[x].node=0;

      if ((current_zone==d->zone || d->zone==0 || nlver==NLVER_5) &&
          ndi[x].net==d->net &&
          ndi[x].node==d->node)
      {
        close(idxfile);
        return (int)offset+x;
      }
    }
  }

  close(idxfile);
  return -1;
}



int ReadNode(NETADDR *d,void *nodeptr)
{
  char temp[PATHLEN];

  int nlfile;

  int nlver;

  word offset;
  word x;

  unsigned long n6size=0L;

  nlver = ngcfg_get_nodelist_version_int();

  if ((offset=NodeExist(d))==(unsigned int)-1)
    return FALSE;
  else
  {
    if (nlver==NLVER_6)
    {
      LangSprintf(temp, sizeof(temp), idxnode, ngcfg_get_string_raw("maximus.net_info_path"));

      x=(word)(fsize(temp)/(long)sizeof(struct _ndi));

      LangSprintf(temp, sizeof(temp), datnode, ngcfg_get_string_raw("maximus.net_info_path"));

      n6size=fsize(temp);
      n6size=n6size/(dword)x;
    }
    else LangSprintf(temp, sizeof(temp), sysnode, ngcfg_get_string_raw("maximus.net_info_path"));

    if ((nlfile=shopen(temp, O_RDONLY | O_BINARY))==-1)
    {
/*    cant_open(temp);*/
      return FALSE;
    }

    lseek(nlfile, ((long)offset)*(nlver==NLVER_6 ? (long)n6size :
          (long)sizeof(struct _node)), SEEK_SET);

    if (read(nlfile, nodeptr, nlver==NLVER_6 ? sizeof(struct _newnode) :
         sizeof(struct _node)) <= 0)
    {
      close(nlfile);
      logit(cantread, temp);
      return FALSE;
    }

    close(nlfile);

    return TRUE;
  }
}

