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

#define MAX_LANG_max_chat
#define MAX_INCL_COMMS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include "prog.h"
#include "mm.h"
#include "max_msg.h"
#include "max_file.h"
#include "dr.h"

/* Undefine the mkdir macro so we can use the real function */
#ifdef mkdir
#undef mkdir
#endif

/* Get node-specific temp directory path */
static void Get_Node_Temp_Path(char *path, size_t maxlen)
{
  snprintf(path, maxlen, "%snode%02x", (char *)ngcfg_get_path("maximus.temp_path"), (int)task_num);
}

/* Create node temp directory if it doesn't exist */
static int Create_Node_Temp_Dir(void)
{
  char path[PATHLEN];
  
  Get_Node_Temp_Path(path, sizeof(path));
  
  /* Create directory if it doesn't exist */
  if (!direxist(path))
  {
    if (mkdir(path, 0755) != 0)
    {
      logit("!Error creating node temp directory '%s': %s", path, strerror(errno));
      return -1;
    }
  }
  
  return 0;
}

/* Write DORINFO1.DEF dropfile */
int Write_Dorinfo1(void)
{
  char path[PATHLEN];
  char firstname[80], lastname[80];
  FILE *f;
  
  if (Create_Node_Temp_Dir() == -1)
    return -1;
  
  Get_Node_Temp_Path(path, sizeof(path));
  strcat(path, "/Dorinfo1.Def");
  
  if ((f = fopen(path, "w")) == NULL)
  {
    logit("!Error creating Dorinfo1.Def: %s", strerror(errno));
    return -1;
  }
  
  /* Split user name into first and last */
  getword(usrname, firstname, ctl_delim, 1);
  getword(usrname, lastname, ctl_delim, 2);
  if (!*lastname)
    strcpy(lastname, "NLN");
  
  fprintf(f, "%s\n", ngcfg_get_string_raw("maximus.system_name"));
  
  /* Sysop name */
  getword((char *)ngcfg_get_string_raw("maximus.sysop"), firstname, ctl_delim, 1);
  getword((char *)ngcfg_get_string_raw("maximus.sysop"), lastname, ctl_delim, 2);
  fprintf(f, "%s\n", firstname);
  fprintf(f, "%s\n", lastname);
  
  /* COM port */
  if (local)
    fprintf(f, "COM0\n");
  else
  {
#if defined(UNIX)
    fprintf(f, "COM%d\n", FileHandle_fromCommHandle(ComGetHandle(hcModem)));
#elif defined(NT)
    fprintf(f, "COM%d\n", ComGetHandle(hcModem));
#elif defined(OS_2)
    fprintf(f, "COM%d\n", ComGetFH(hcModem));
#else
    fprintf(f, "COM%d\n", port + 1);
#endif
  }
  
  fprintf(f, "%lu BAUD,N,8,1\n", baud);
  fprintf(f, " 0\n");  /* Not networked */
  
  /* User name */
  getword(usrname, firstname, ctl_delim, 1);
  getword(usrname, lastname, ctl_delim, 2);
  cstrupr(firstname);
  cstrupr(lastname);
  fprintf(f, "%s\n", firstname);
  fprintf(f, "%s\n", lastname);
  
  fprintf(f, "%s\n", usr.city);
  fprintf(f, "%d\n", usr.video == GRAPH_TTY ? 0 : 1);
  fprintf(f, "%u\n", usr.priv);
  fprintf(f, "%d\n", timeleft());
  fprintf(f, "-1\n");  /* FOSSIL */
  
  fclose(f);
  logit("@Created Dorinfo1.Def in %s", path);
  return 0;
}

/* Write DOOR.SYS dropfile */
int Write_DoorSys(void)
{
  char path[PATHLEN];
  FILE *f;
  
  if (Create_Node_Temp_Dir() == -1)
    return -1;
  
  Get_Node_Temp_Path(path, sizeof(path));
  strcat(path, "/Door.Sys");
  
  if ((f = fopen(path, "w")) == NULL)
  {
    logit("!Error creating Door.Sys: %s", strerror(errno));
    return -1;
  }
  
  /* COM port */
  if (local)
    fprintf(f, "COM0:\n");
  else
  {
#if defined(UNIX)
    fprintf(f, "COM%d:\n", FileHandle_fromCommHandle(ComGetHandle(hcModem)));
#elif defined(NT)
    fprintf(f, "COM%d:\n", ComGetHandle(hcModem));
#elif defined(OS_2)
    fprintf(f, "COM%d:\n", ComGetFH(hcModem));
#else
    fprintf(f, "COM%d:\n", port + 1);
#endif
  }
  
  fprintf(f, "%lu\n", baud);
  fprintf(f, "8\n");  /* Data bits */
  fprintf(f, "%d\n", task_num);
  fprintf(f, "N\n");  /* DTE locked */
  fprintf(f, "Y\n");  /* Screen display */
  fprintf(f, "Y\n");  /* Printer toggle */
  fprintf(f, "Y\n");  /* Page bell */
  fprintf(f, "Y\n");  /* Caller alarm */
  fprintf(f, "%s\n", usrname);
  fprintf(f, "%s\n", usr.city);
  fprintf(f, "%s\n", usr.phone);
  fprintf(f, "%s\n", usr.phone);
  fprintf(f, "%s\n", usr.pwd);
  fprintf(f, "%u\n", usr.priv);
  fprintf(f, "%u\n", usr.times);
  fprintf(f, "01/01/90\n");  /* Last date called */
  fprintf(f, "%ld\n", (long)(timeoff - time(NULL)));
  fprintf(f, "%d\n", timeleft());
  fprintf(f, "%s\n", usr.video == GRAPH_TTY ? "NG" : "GR");
  fprintf(f, "%d\n", usr.len);
  fprintf(f, "N\n");  /* Expert mode */
  fprintf(f, "1,2,3,4,5,6,7\n");  /* Conferences */
  fprintf(f, "1\n");  /* Conf last in */
  fprintf(f, "01/01/99\n");  /* Expiration */
  fprintf(f, "%u\n", usr.lastread_ptr);
  fprintf(f, "X\n");  /* Default protocol */
  fprintf(f, "0\n");
  fprintf(f, "0\n");
  fprintf(f, "0\n");
  fprintf(f, "9999\n");
  
  fclose(f);
  logit("@Created Door.Sys in %s", path);
  return 0;
}

/* Write CHAIN.TXT dropfile */
int Write_ChainTxt(void)
{
  char path[PATHLEN];
  FILE *f;
  
  if (Create_Node_Temp_Dir() == -1)
    return -1;
  
  Get_Node_Temp_Path(path, sizeof(path));
  strcat(path, "/Chain.Txt");
  
  if ((f = fopen(path, "w")) == NULL)
  {
    logit("!Error creating Chain.Txt: %s", strerror(errno));
    return -1;
  }
  
  fprintf(f, "%u\n", usr.lastread_ptr);
  fprintf(f, "%s\n", firstname);
  fprintf(f, "%s\n", (char *)firstchar(usrname, ctl_delim, 2) ?: "NLN");
  fprintf(f, "\n");  /* Callsign/handle */
  fprintf(f, "%u\n", usr.priv);
  fprintf(f, "%d\n", timeleft());
  fprintf(f, "%s\n", usr.video == GRAPH_TTY ? "0" : "1");
  fprintf(f, "%d\n", task_num);
  
  /* COM port */
  if (local)
    fprintf(f, "0\n");
  else
  {
#if defined(UNIX)
    fprintf(f, "%d\n", FileHandle_fromCommHandle(ComGetHandle(hcModem)));
#elif defined(NT)
    fprintf(f, "%d\n", ComGetHandle(hcModem));
#elif defined(OS_2)
    fprintf(f, "%d\n", ComGetFH(hcModem));
#else
    fprintf(f, "%d\n", port + 1);
#endif
  }
  
  fprintf(f, "%lu\n", baud);
  fprintf(f, "%s\n", ngcfg_get_string_raw("maximus.system_name"));
  fprintf(f, "%s\n", firstname);
  fprintf(f, "%s\n", (char *)firstchar((char *)ngcfg_get_string_raw("maximus.sysop"), ctl_delim, 2) ?: "NLN");
  fprintf(f, "\n");  /* Sysop callsign */
  fprintf(f, "00:00\n");  /* Event time */
  fprintf(f, "N\n");  /* Error correcting */
  fprintf(f, "N\n");  /* ANSI in NG mode */
  fprintf(f, "Y\n");  /* Record locking */
  fprintf(f, "7\n");  /* Default color */
  fprintf(f, "%u\n", usr.times);
  fprintf(f, "01/01/90\n");  /* Last date */
  fprintf(f, "%ld\n", (long)(timeoff - time(NULL)));
  fprintf(f, "9999\n");  /* Daily file limit */
  fprintf(f, "0\n");  /* Files downloaded today */
  fprintf(f, "%u\n", (unsigned int)usr.up);
  fprintf(f, "%u\n", (unsigned int)usr.down);
  fprintf(f, "8\n");  /* User comment */
  fprintf(f, "0\n");  /* Doors opened */
  fprintf(f, "0\n");  /* Messages left */
  
  fclose(f);
  logit("@Created Chain.Txt in %s", path);
  return 0;
}

/* Write DOOR32.SYS dropfile */
int Write_Door32Sys(void)
{
  char path[PATHLEN];
  FILE *f;
  
  if (Create_Node_Temp_Dir() == -1)
    return -1;
  
  Get_Node_Temp_Path(path, sizeof(path));
  strcat(path, "/door32.sys");
  
  if ((f = fopen(path, "w")) == NULL)
  {
    logit("!Error creating door32.sys: %s", strerror(errno));
    return -1;
  }
  
  /* Comm type: 0=local, 2=telnet/socket */
  fprintf(f, "%d\n", local ? 0 : 2);
  
  /* Comm handle */
  if (local)
    fprintf(f, "0\n");
  else
  {
#if defined(UNIX)
    fprintf(f, "%d\n", FileHandle_fromCommHandle(ComGetHandle(hcModem)));
#elif defined(NT)
    fprintf(f, "%d\n", ComGetHandle(hcModem));
#elif defined(OS_2)
    fprintf(f, "%d\n", ComGetFH(hcModem));
#else
    fprintf(f, "0\n");
#endif
  }
  
  fprintf(f, "%lu\n", baud);
  fprintf(f, "%s\n", ngcfg_get_string_raw("maximus.system_name"));
  fprintf(f, "%u\n", usr.lastread_ptr);
  fprintf(f, "%s\n", usrname);
  fprintf(f, "%s\n", usrname);  /* Handle/alias */
  fprintf(f, "%u\n", usr.priv);
  fprintf(f, "%d\n", timeleft());
  fprintf(f, "%s\n", usr.video == GRAPH_TTY ? "ASCII" : "ANSI");
  fprintf(f, "%d\n", task_num);
  
  fclose(f);
  logit("@Created door32.sys in %s", path);
  return 0;
}

/* Clean up node temp directory */
void Clean_Node_Temp_Dir(void)
{
  char path[PATHLEN];
  char filepath[PATHLEN];
  FFIND *ff;
  
  Get_Node_Temp_Path(path, sizeof(path));
  
  if (!direxist(path))
    return;
  
  logit("@Cleaning node temp directory: %s", path);
  
  /* Delete all files in the directory */
  snprintf(filepath, sizeof(filepath), "%s/*", path);
  
  if ((ff = FindOpen(filepath, 0)) != NULL)
  {
    do
    {
      /* Skip . and .. */
      if (strcmp(ff->szName, ".") == 0 || strcmp(ff->szName, "..") == 0)
        continue;
      
      snprintf(filepath, sizeof(filepath), "%s/%s", path, ff->szName);
      
      if (unlink(filepath) == 0)
        logit("@Deleted: %s", filepath);
      else
        logit("!Error deleting %s: %s", filepath, strerror(errno));
    }
    while (FindNext(ff) == 0);
    
    FindClose(ff);
  }
}

/* Write all dropfiles */
int Write_All_Dropfiles(void)
{
  int ret = 0;
  
  logit("@Writing dropfiles to node temp directory");
  
  if (Write_Dorinfo1() == -1)
    ret = -1;
  
  if (Write_DoorSys() == -1)
    ret = -1;
  
  if (Write_ChainTxt() == -1)
    ret = -1;
  
  if (Write_Door32Sys() == -1)
    ret = -1;
  
  return ret;
}
