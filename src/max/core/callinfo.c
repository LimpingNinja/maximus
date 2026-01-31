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

/*
 * Caller information API
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <errno.h>
#include "prog.h"
#include "mm.h"
#include "dropfile.h"

void ci_login(void)
{
  Get_Dos_Date(&sci.login);
  sci.logon_priv=usr.priv;
  sci.logon_xkeys=usr.xkeys;
  sci.calls=usr.times+1;
  
  /* Clean up node temp directory on login */
  Clean_Node_Temp_Dir();
}

void ci_init(void)
{
  const char *caller_log;

  caller_log = ngcfg_get_path("maximus.file_callers");
  logit("@ci_init: caller_log='%s'", caller_log ? caller_log : "");
  if (caller_log && *caller_log)
  {
    memset(&sci, 0, sizeof sci);
    strcpy(sci.name, usrname);
    sci.task=task_num;
    ci_login();
    logit("@ci_init: initialized sci.name='%s' task=%d", sci.name, sci.task);
  }
  else
  {
    logit("@ci_init: caller_log is EMPTY - skipping init");
  }
}

void ci_filename(char * buf)
{
  char temp[PATHLEN];
  const char *caller_log;

  *buf='\0';

  caller_log = ngcfg_get_path("maximus.file_callers");
  if (caller_log && *caller_log)
  {
    char *p;

    strcpy(temp, caller_log);
    Convert_Star_To_Task(temp);
    Parse_Outside_Cmd(temp,buf);
    p=strrchr(buf, PATH_DELIM);
    if (p==NULL)
      p=buf;
    if (strchr(p,'.')==NULL)
      strcat(buf, dotbbs);
  }
}

void ci_save(void)
{
  char temp[PATHLEN];

  logit("@ci_save: called");
  ci_filename(temp);
  logit("@ci_save: filename='%s' sci.name='%s'", temp, sci.name);
  
  if (*sci.name && *temp)
  {
    int fd;

    /* Complete caller information */

    strcpy(sci.name, usrname);
    strcpy(sci.city, usr.city);
    sci.task=task_num;
    Get_Dos_Date(&sci.logoff);
    sci.logoff_priv=usr.priv;
    sci.logoff_xkeys=usr.xkeys;

    logit("@ci_save: opening '%s' for append", temp);
    fd=shopen(temp, O_RDWR|O_CREAT|O_APPEND|O_BINARY);
    if (fd!=-1)
    {
      write(fd, (char *)&sci, sizeof sci);
      close(fd);
      logit("@ci_save: SUCCESS wrote %d bytes to '%s'", (int)sizeof(sci), temp);
    }
    else
    {
      logit("@ci_save: FAILED to open '%s' errno=%d", temp, errno);
    }
  }
  else
  {
    logit("@ci_save: SKIPPED - sci.name='%s' temp='%s'", sci.name, temp);
  }
}


