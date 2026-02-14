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
static char rcs_id[]="$Id: ued_disp.c,v 1.4 2004/01/28 06:38:11 paltas Exp $";
#pragma on(unreferenced)
#endif

/*# name=Internal user editor (screen-display routines)
*/

#define MAX_INCL_COMMS

#define MAX_LANG_global
#define MAX_LANG_m_area
#define MAX_LANG_max_ued
#define MAX_LANG_sysop
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 #include "libmaxcfg.h"
#include "prog.h"
#include "mm.h"
#include "ued.h"
#include "ued_disp.h"

static const char *near ngcfg_lang_file_name(byte idx)
{
  MaxCfgVar v;
  MaxCfgVar it;
  size_t cnt = 0;

  if (ng_cfg && maxcfg_toml_get(ng_cfg, "general.language.lang_file", &v) == MAXCFG_OK && v.type == MAXCFG_VAR_STRING_ARRAY)
  {
    if (maxcfg_var_count(&v, &cnt) == MAXCFG_OK && (size_t)idx < cnt)
    {
      if (maxcfg_toml_array_get(&v, (size_t)idx, &it) == MAXCFG_OK && it.type == MAXCFG_VAR_STRING && it.v.s && *it.v.s)
        return it.v.s;
    }
  }

  return "";
}

static int near MKD(void);


/*                1               2               3               4
   123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
1  User ------------------------------- Last call                   -----------Ŀ
2   - Name      123456789012345678 Alias      1234567890123456789012345         -
3   - City      Kingston           VoicePhone 123456789012345 Sex  Female       -
4   - Password  1234567890123456   DataPhone  (613)634-3058   Bday 74-03-24     -
5  A)ccess -----------------------------------Ŀ    S)ettings -----------------Ŀ
6   - Priv level AsstSysop   Credit    65500   -     - Width      80            -
7   - Keys       12345678901 Debit     7952    -     - Length     25            -
8   - Group Num  0           Used Pts  12345678-     - Nulls      0             -
9   - Alloc Pts  1234567890  ShowUlist YES     -     - Msg Area   MAX.MUFFIN    -
a   - Nerd       NO                            -     - File Area  12345678901234-
b  I)nformation ----------------------------------Ŀ - Video Mode AVATAR        -
c   - DL (all) 800K / 52222    Cur. time  12345    - - Help       REGULAR       -
d   - Today DL 0K / 0          Added time 12345    - - !Language  English       -
e   - Uploads  1234567K / 2233 # calls    7723     - - Protocol   Zmodem        -
f   - PostMsgs 1234            ReadMsgs   22       - - Compress   1234567890123 -
10  - 1stCall  03/22/94        !PwdDate   03/22/94 -
11 F)lags ----------------------------------------Ŀ E)xpiry ------------------Ŀ
12  - Hotkeys YES  IBM Chars    YES  ScrnClr   YES -  - Expire by 1234567890    -
13  - MaxEd   YES  Pause (More) YES  AvailChat  NO -  - Action    12345678901234-
14  - Tabs    YES  CalledBefore YES  FSR       YES -  - Date      NONE          -
15  - RIP      NO                                  -
16
17  Select: Aasdf)
18          Foo)
*/


static char * near Expire_By(struct _usr *user)
{
  if (user->xp_flag & XFLAG_EXPDATE)
    return ued_xp_date;
  else if (user->xp_flag & XFLAG_EXPMINS)
    return ued_xp_mins;
  else
    return ued_xp_none;
}

static char * near Expire_Action(struct _usr *user,char *temp)
{
  if (user->xp_flag & XFLAG_DEMOTE)
  {
    char ptmp[16];

    LangSprintf(temp, PATHLEN, ued_xp_demote, privstr(user->xp_priv,ptmp));
    return temp;
  }

  if (user->xp_flag & XFLAG_AXE)
    return ued_xp_hangup;

  return ued_xp_none;
}

static char * near Expire_At(struct _usr *user,char *temp)
{
  if (user->xp_flag & XFLAG_EXPDATE)
  {
    FileDateFormat(&user->xp_date, temp);
    return temp;
  }

  if (user->xp_flag & XFLAG_EXPMINS)
  {
    { char _xm[16]; snprintf(_xm, sizeof(_xm), "%ld", (long)user->xp_mins);
      LangSprintf(temp, PATHLEN, ued_xp_minutes, _xm); }
    return temp;
  }

  return ued_xp_none;
}



/* Display all of the ued_ssxxx strings from the language file */

void DrawUserScreen(void)
{
  int i;
  char *str;

  for (i=1; i <= 22; i++)
  {
    char sskey[32];
    snprintf(sskey, sizeof(sskey), "max_ued.ued_ss%d", i);
    const char *ss = maxlang_get(g_current_lang, sskey);
    if (*ss)
      Puts((char *)ss);
  }
}



static char * near Sex(byte sex)
{
  switch (sex)
  {
    case SEX_MALE:    return sex_male;
    case SEX_FEMALE:  return sex_female;
    default:          return sex_unknown;
  }
}




/* Display the user's date of birth in a sensible format */

static char * near DOB(struct _usr *pusr, char *out)
{
  int ds = ngcfg_get_int("general.session.date_style");

  switch (ds)
  {
    default:
      sprintf(out,
              date_str,
              pusr->dob_month,
              pusr->dob_day,
              pusr->dob_year % 100);
      break;

    case 1:
      sprintf(out,
              date_str,
              pusr->dob_day,
              pusr->dob_month,
              pusr->dob_year % 100);
      break;

    case 2:
    case 3:
      sprintf(out,
              ds==2 ? date_str : datestr,
              pusr->dob_year % 100,
              pusr->dob_month,
              pusr->dob_day);
  }

  return out;
}


/* Display a user record on-scren */

void DisplayUser(void)
{
  struct _arcinfo *ar;
  char temp[125];
  char ptmp[12];

  Puts(WHITE);

  LangPrintf(ued_spermflag,
               eqstri(usr.name, user.name) ? ued_sstatcur
             : (user.delflag & UFLAG_DEL)  ? ued_sstatdel
             : (user.delflag & UFLAG_PERM) ? ued_sstatprm
                                           : ued_sstatblank);

  if (MKD()) goto Dump;
  LangPrintf(ued_slastcall,    sc_time(&user.ludate, temp));
  if (MKD()) goto Dump;
  LangPrintf(ued_sname,        user.name);
  if (MKD()) goto Dump;
  LangPrintf(ued_scity,        user.city);
  if (MKD()) goto Dump;
  /* Always show [Encrypted] - never expose passwords to sysop */
  LangPrintf(ued_spwd,         brackets_encrypted);
  if (MKD()) goto Dump;
  LangPrintf(ued_salias,       user.alias);
  if (MKD()) goto Dump;
  LangPrintf(ued_svoicephone,  user.phone);
  if (MKD()) goto Dump;
  LangPrintf(ued_sdataphone,   user.dataphone);
  if (MKD()) goto Dump;
  LangPrintf(ued_ssex,         Sex(user.sex));
  if (MKD()) goto Dump;
  LangPrintf(ued_sdob,         DOB(&user, temp));
  if (MKD()) goto Dump;
  LangPrintf(ued_spriv,        privstr(user.priv,ptmp));
  if (MKD()) goto Dump;
  LangPrintf(ued_skeys,        Keys(user.xkeys));
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%u", (unsigned)user.group);
    LangPrintf(ued_sgroup, _ib); }
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%ld", (long)user.point_credit);
    LangPrintf(ued_sallocpts, _ib); }
  if (MKD()) goto Dump;
  LangPrintf(ued_snerd,        Yes_or_No((user.bits  & BITS_NERD)));
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%ld", (long)user.credit);
    LangPrintf(ued_scredit, _ib); }
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%ld", (long)user.debit);
    LangPrintf(ued_sdebit, _ib); }
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%ld", (long)user.point_debit);
    LangPrintf(ued_susedpts, _ib); }
  if (MKD()) goto Dump;
  LangPrintf(ued_sulistshow,   Yes_or_No(!(user.bits & BITS_NOULIST)));
  if (MKD()) goto Dump;

  { char _kb[16], _nf[16];
    snprintf(_kb, sizeof(_kb), "%ld", (long)user.down);
    snprintf(_nf, sizeof(_nf), "%u", (unsigned)user.ndown);
    LangSprintf(temp, sizeof(temp), ued_sxfertemplate, _kb, _nf); }
  LangPrintf(ued_sdlall, temp);
  if (MKD()) goto Dump;

  { char _kb[16], _nf[16];
    snprintf(_kb, sizeof(_kb), "%ld", (long)user.downtoday);
    snprintf(_nf, sizeof(_nf), "%u", (unsigned)user.ndowntoday);
    LangSprintf(temp, sizeof(temp), ued_sxfertemplate, _kb, _nf); }
  LangPrintf(ued_sdltoday,     temp);
  if (MKD()) goto Dump;

  { char _kb[16], _nf[16];
    snprintf(_kb, sizeof(_kb), "%ld", (long)user.up);
    snprintf(_nf, sizeof(_nf), "%u", (unsigned)user.nup);
    LangSprintf(temp, sizeof(temp), ued_sxfertemplate, _kb, _nf); }
  LangPrintf(ued_sup,          temp);
  if (MKD()) goto Dump;

  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%u", (unsigned)user.msgs_posted);
    LangPrintf(ued_sposted, _ib); }
  if (MKD()) goto Dump;

  CreateDate(temp, &user.date_1stcall);
  LangPrintf(ued_s1stcall,     temp);
  if (MKD()) goto Dump;

  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%d", user.time);
    LangPrintf(ued_stimetoday, _ib); }
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%d", user.time_added);
    LangPrintf(ued_stimeadded, _ib); }
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%u", (unsigned)user.times);
    LangPrintf(ued_stimes, _ib); }
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%u", (unsigned)user.msgs_read);
    LangPrintf(ued_sreadmsgs, _ib); }
  if (MKD()) goto Dump;

  CreateDate(temp, &user.date_pwd_chg);
  LangPrintf(ued_spwdchg,      temp);
  if (MKD()) goto Dump;

  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%u", (unsigned)user.width);
    LangPrintf(ued_swidth, _ib); }
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%u", (unsigned)user.len);
    LangPrintf(ued_slength, _ib); }
  if (MKD()) goto Dump;
  { char _ib[32]; snprintf(_ib, sizeof(_ib), "%u", (unsigned)user.nulls);
    LangPrintf(ued_snulls, _ib); }
  if (MKD()) goto Dump;
  LangPrintf(ued_slastmarea,   user.msg);
  if (MKD()) goto Dump;
  LangPrintf(ued_slastfarea,   user.files);
  if (MKD()) goto Dump;
  LangPrintf(ued_svideo,       Graphics_Mode(user.video));
  if (MKD()) goto Dump;
  LangPrintf(ued_shelp,        Help_Level(user.help));
  if (MKD()) goto Dump;
  {
    const char *lname = ngcfg_lang_file_name((byte)user.lang);
    LangPrintf(ued_slang, (char *)lname);
  }
  if (MKD()) goto Dump;
  LangPrintf(ued_sproto,       Protocol_Name(user.def_proto, temp));
  if (MKD()) goto Dump;

  ar=UserAri(user.compress);
  LangPrintf(ued_scompress,    ar ? ar->arcname : proto_none);

  if (MKD()) goto Dump;
  LangPrintf(ued_shotkeys,     Yes_or_No((user.bits  & BITS_HOTKEYS)));
  if (MKD()) goto Dump;
  LangPrintf(ued_smaxed,       Yes_or_No(!(user.bits2& BITS2_BORED)));
  if (MKD()) goto Dump;
  LangPrintf(ued_stabs,        Yes_or_No((user.bits  & BITS_TABS)));
  if (MKD()) goto Dump;
  LangPrintf(ued_srip,         Yes_or_No((user.bits  & BITS_RIP)));
  if (MKD()) goto Dump;
  LangPrintf(ued_sibmchars,    Yes_or_No((user.bits2 & BITS2_IBMCHARS)));
  if (MKD()) goto Dump;
  LangPrintf(ued_spause,       Yes_or_No((user.bits2 & BITS2_MORE)));
  if (MKD()) goto Dump;
  LangPrintf(ued_scalledbefore,Yes_or_No((user.bits2 & BITS2_CONFIGURED)));
  if (MKD()) goto Dump;
  LangPrintf(ued_sscrnclr,     Yes_or_No((user.bits2 & BITS2_CLS)));
  if (MKD()) goto Dump;
  LangPrintf(ued_schatavail,   Yes_or_No(!(user.bits & BITS_NOTAVAIL)));
  if (MKD()) goto Dump;
  LangPrintf(ued_sfsr,         Yes_or_No((user.bits  & BITS_FSR)));
  if (MKD()) goto Dump;
  LangPrintf(ued_sexpireby,    Expire_By(&user));
  if (MKD()) goto Dump;
  LangPrintf(ued_sexpireact,   Expire_Action(&user, temp));
  if (MKD()) goto Dump;
  LangPrintf(ued_sexpiredate,  Expire_At(&user, temp));
  return;

Dump:

  mdm_dump(DUMP_OUTPUT);
  ResetAttr();
  return;
}


static int near MKD(void)
{
  if (Mdm_keyp() && (usr.bits & BITS_HOTKEYS))
    return TRUE;
  else
    return FALSE;
}


/* Display the password, optionally echoing '.'s for each character */

char * Show_Pwd(char *pwd,char *ret,char echo)
{
  char *s, *p;

  for (s=pwd,p=ret;*s;s++)
    if (echo)
      *p++=echo;
    else *p++=*s;

  *p='\0';

  return ret;
}



