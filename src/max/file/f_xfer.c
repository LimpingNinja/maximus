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

#ifndef __GNUC__
#pragma off(unreferenced)
static char rcs_id[]="$Id: f_xfer.c,v 1.3 2004/01/27 21:00:29 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=File area routines: Required functions for both ULing and DLing
*/

#define MAX_LANG_f_area
#define MAX_LANG_global
#define MAX_LANG_m_area
#include <ctype.h>
#include <stdio.h>
#include <mem.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "alc.h"
#include "libmaxcfg.h"
#ifdef BINK_PROTOCOLS
  #include "zmodem.h"
#endif
#include "prog.h"
#include "max_file.h"
#include "fb.h"
#include "f_idx.h"


static int near ngcfg_get_protocol_exitlevel(int protocol)
{
  MaxCfgVar protos;
  MaxCfgVar item;
  MaxCfgVar v;
  size_t cnt = 0;

  if (protocol < 0 || protocol >= MAX_EXTERNP)
    return 0;

  if (ng_cfg &&
      maxcfg_toml_get(ng_cfg, "general.protocol.protocol", &protos) == MAXCFG_OK &&
      protos.type == MAXCFG_VAR_TABLE_ARRAY &&
      maxcfg_var_count(&protos, &cnt) == MAXCFG_OK)
  {
    for (size_t i = 0; i < cnt; i++)
    {
      int idx = -1;

      if (maxcfg_toml_array_get(&protos, i, &item) != MAXCFG_OK || item.type != MAXCFG_VAR_TABLE)
        continue;

      if (maxcfg_toml_table_get(&item, "index", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_INT)
        idx = v.v.i;

      if (idx != protocol)
        continue;

      if (maxcfg_toml_table_get(&item, "exitlevel", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_BOOL)
        return v.v.b ? 1 : 0;

      break;
    }
  }

  return 0;
}


int File_Get_Protocol(sword *protocol, int chng, int need_nonexit)
{
  char temp[PATHLEN];
  struct _proto_str *ps;
  char *s;

  word max, prot;
  byte ch;


  if (!chng && usr.def_proto != PROTOCOL_NONE)
  {
    if (!need_nonexit || usr.def_proto < 0 ||
        !ngcfg_get_protocol_exitlevel(usr.def_proto))
    {
      *protocol=usr.def_proto;
      return 0;
    }
  }
  
  *protocol=PROTOCOL_NONE;

  if (! *linebuf)
  {
    if (*ngcfg_get_path("general.display_files.protocol_dump"))
      Display_File(0,NULL,ngcfg_get_path("general.display_files.protocol_dump"));
    else
    {
      if (!chng)
        Puts(avail_proto); /* "Available protocols:" */

      /* Now, calculate the minimum required width of the box... */

      max=9;

      for (prot=0; prot < MAX_EXTERNP; prot++)
        if (!need_nonexit || !ngcfg_get_protocol_exitlevel(prot))
        {
          if (strlen(s=Protocol_Name(prot, temp)) > max)
            max=strlen(s);
        }


      /* Display the top of the box */

      { char _ib[16]; snprintf(_ib, sizeof(_ib), "%02d", max+3);
        LangPrintf(proto_box_top, _ib); }

      /* Print the middle of the box */
        
      for (ps=intern_proto; ps->name; ps++)
        if (*ps->name)
        {
          if (no_zmodem && *ps->name=='Z')
            continue;

          { char _cb[2] = { *ps->name, '\0' };
            char _w1[16], _w2[16];
            snprintf(_w1, sizeof(_w1), "%d", max-1);
            snprintf(_w2, sizeof(_w2), "%d", max-1);
            LangPrintf(proto_box_mid, _cb, _w1, _w2, ps->name+1); }
        }

      for (prot=0; prot < MAX_EXTERNP; prot++)
        if (!need_nonexit || !ngcfg_get_protocol_exitlevel(prot))
          if (*(s=Protocol_Name(prot, temp)))
            { char _cb[2] = { (char)toupper(*s), '\0' };
              char _w1[16], _w2[16];
              snprintf(_w1, sizeof(_w1), "%d", max-1);
              snprintf(_w2, sizeof(_w2), "%d", max-1);
              LangPrintf(proto_box_mid, _cb, _w1, _w2, s+1); }

      { char _cb[2] = { chng ? *proto_none : *proto_quit, '\0' };
        char _w1[16], _w2[16];
        snprintf(_w1, sizeof(_w1), "%d", max-1);
        snprintf(_w2, sizeof(_w2), "%d", max-1);
        LangPrintf(proto_box_mid, _cb, _w1, _w2,
                   (chng ? proto_none : proto_quit)+1); }

      { char _ib[16]; snprintf(_ib, sizeof(_ib), "%02d", max+3);
        LangPrintf(proto_box_bot, _ib); }
    }
  }

  ch=(byte)toupper(KeyGetRNP(select_p));

  if (ch=='Q' || (ch=='N' && chng) || ch=='\r' || ch=='\x00')
    return -1;

  for (ps=intern_proto; ps->name; ps++)
  {
    if (no_zmodem && *ps->name=='Z')
      continue;

    if (*ps->name==ch)
    {
      *protocol=ps->num;
      return 0;
    }
  }

  for (prot=0; prot < MAX_EXTERNP; prot++)
    if (!need_nonexit || !ngcfg_get_protocol_exitlevel(prot))
      if (ch==(byte)toupper(*Protocol_Name(prot, temp)))
      {
        *protocol=(int)prot;
        return 0;
      }

  *linebuf='\0';
  { char _cb[2] = { (char)ch, '\0' };
    LangPrintf(dontunderstand, _cb); }
  return -1;
}









/* This file is only written during file transfers, so some other part      *
 * of a multitasking system can tell that we're not free for a two-way      *
 * chat, or whatever.                                                       */

void Open_OpusXfer(FILE **xferinfo)
{
  char xname[PATHLEN];

  sprintf(xname, opusxfer_name, original_path, task_num);

  if ((*xferinfo=shfopen(xname, fopen_write, O_WRONLY|O_TRUNC|O_CREAT|O_NOINHERIT))==NULL)
    cant_open(xname);
}





void Close_OpusXfer(FILE **xferinfo)
{
  if (*xferinfo)
  {
    fputc('\n', *xferinfo);
    fclose(*xferinfo);
  }

  *xferinfo=NULL;
}





void Delete_OpusXfer(FILE **xferinfo)
{
  char xname[PATHLEN];

  if (*xferinfo)
    fclose(*xferinfo);

  *xferinfo=NULL;

  sprintf(xname, opusxfer_name, original_path, task_num);
  unlink(xname);
}




byte Get_Protocol_Letter(sword protocol)
{
  switch (protocol)
  {
    default:
    case PROTOCOL_XMODEM:     return 'X';
    case PROTOCOL_XMODEM1K:   return '1';
    case PROTOCOL_YMODEM:     return 'Y';
    case PROTOCOL_YMODEMG:    return 'G';
    case PROTOCOL_SEALINK:    return 'S';
    case PROTOCOL_ZMODEM:     return 'Z';
  }
}








#ifdef BINK_PROTOCOLS
char *zalloc(void)
{
  byte *sptr;

  if ((sptr=malloc(WAZOOMAX+16))==NULL)
    logit("!Z-MEMOVFL");

  return sptr;
}
#endif

/* Wait 'x' seconds for user to either press <esc> or press <enter> */

word Shall_We_Continue(word timeout, char *do_what)
{
  word pause, ret;
  int ch;

  long tmr;

  ret=TRUE;

  Putc('\n');
  
  { char _ib[16]; snprintf(_ib, sizeof(_ib), "%d", (pause=timeout));
    LangPrintf(pause_msg, _ib, do_what); }
    
  while (pause-- > 0)
  {
    { char _ib[16]; snprintf(_ib, sizeof(_ib), "%d", pause);
      LangPrintf(pause_time, _ib); }
    
    tmr=timerset(100);

    vbuf_flush();

    while (!timeup(tmr) && !Mdm_keyp())
      Giveaway_Slice();

    if (Mdm_keyp())
    {
      ch=Mdm_getcw();
      
      if (ch=='\x0d')       /* C/R */
        break;
      else if (ch=='\x1b')  /* ESC */
      {
/*        Puts(xferaborted);*/
        ret=FALSE;
        break;
      }
    }
  }

  Puts("\r" CLEOL);
  WhiteN();
  return ret;
}


