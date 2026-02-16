/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 *
 * Modifications Copyright (C) 2025 Kevin Morgan (Limping Ninja)
 * https://github.com/LimpingNinja
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

#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "prog.h"
#include "max.h"
#include "userapi.h"
#include "uni.h"
#include "compiler.h"
#include "libmaxdb.h"

#ifdef OS_2
  #define INCL_DOS
  #include "pos2.h"
#endif

static int _use_sqlite_backend(void)
{
  return 1;
}

/* Conversion helpers: struct _usr <-> MaxDBUser */
static void _convert_usr_to_maxdbuser(const struct _usr *usr, MaxDBUser *dbuser, int id)
{
  memset(dbuser, 0, sizeof(MaxDBUser));
  
  dbuser->id = id;
  
  /* Identity */
  strncpy(dbuser->name, (char*)usr->name, sizeof(dbuser->name) - 1);
  strncpy(dbuser->city, (char*)usr->city, sizeof(dbuser->city) - 1);
  strncpy(dbuser->alias, (char*)usr->alias, sizeof(dbuser->alias) - 1);
  strncpy(dbuser->phone, (char*)usr->phone, sizeof(dbuser->phone) - 1);
  strncpy(dbuser->dataphone, (char*)usr->dataphone, sizeof(dbuser->dataphone) - 1);
  
  /* Authentication */
  memcpy(dbuser->pwd, usr->pwd, sizeof(dbuser->pwd));
  dbuser->pwd_encrypted = (usr->bits & BITS_ENCRYPT) ? 1 : 0;
  
  /* Demographics */
  dbuser->dob_year = usr->dob_year;
  dbuser->dob_month = usr->dob_month;
  dbuser->dob_day = usr->dob_day;
  dbuser->sex = usr->sex;
  
  /* Access control */
  dbuser->priv = usr->priv;
  dbuser->xkeys = usr->xkeys;
  
  /* Expiry */
  dbuser->xp_priv = usr->xp_priv;
  dbuser->xp_date = usr->xp_date;
  dbuser->xp_mins = usr->xp_mins;
  dbuser->xp_flag = usr->xp_flag;
  
  /* Statistics */
  dbuser->times = usr->times;
  dbuser->call = usr->call;
  dbuser->msgs_posted = usr->msgs_posted;
  dbuser->msgs_read = usr->msgs_read;
  dbuser->nup = usr->nup;
  dbuser->ndown = usr->ndown;
  dbuser->ndowntoday = usr->ndowntoday;
  dbuser->up = usr->up;
  dbuser->down = usr->down;
  dbuser->downtoday = usr->downtoday;
  
  /* Session info */
  dbuser->ludate = usr->ludate;
  dbuser->date_1stcall = usr->date_1stcall;
  dbuser->date_pwd_chg = usr->date_pwd_chg;
  dbuser->date_newfile = usr->date_newfile;
  dbuser->time = usr->time;
  dbuser->time_added = usr->time_added;
  dbuser->timeremaining = usr->timeremaining;
  
  /* Preferences */
  dbuser->video = usr->video;
  dbuser->lang = usr->lang;
  dbuser->width = usr->width;
  dbuser->len = usr->len;
  dbuser->help = usr->help;
  dbuser->nulls = usr->nulls;
  dbuser->def_proto = usr->def_proto;
  dbuser->compress = usr->compress;
  
  /* Area tracking */
  dbuser->lastread_ptr = usr->lastread_ptr;
  strncpy(dbuser->msg, (char*)usr->msg, sizeof(dbuser->msg) - 1);
  strncpy(dbuser->files, (char*)usr->files, sizeof(dbuser->files) - 1);
  
  /* Credits */
  dbuser->credit = usr->credit;
  dbuser->debit = usr->debit;
  dbuser->point_credit = usr->point_credit;
  dbuser->point_debit = usr->point_debit;
  
  /* Flags */
  dbuser->bits = usr->bits;
  dbuser->bits2 = usr->bits2;
  dbuser->delflag = usr->delflag;
  
  /* Misc */
  dbuser->group = usr->group;
  dbuser->extra = usr->extra;
}

static void _convert_maxdbuser_to_usr(const MaxDBUser *dbuser, struct _usr *usr)
{
  memset(usr, 0, sizeof(struct _usr));
  
  /* Identity */
  strncpy((char*)usr->name, dbuser->name, sizeof(usr->name) - 1);
  strncpy((char*)usr->city, dbuser->city, sizeof(usr->city) - 1);
  strncpy((char*)usr->alias, dbuser->alias, sizeof(usr->alias) - 1);
  strncpy((char*)usr->phone, dbuser->phone, sizeof(usr->phone) - 1);
  strncpy((char*)usr->dataphone, dbuser->dataphone, sizeof(usr->dataphone) - 1);
  
  /* Authentication */
  memcpy(usr->pwd, dbuser->pwd, sizeof(usr->pwd));
  if (dbuser->pwd_encrypted)
    usr->bits |= BITS_ENCRYPT;
  
  /* Demographics */
  usr->dob_year = dbuser->dob_year;
  usr->dob_month = dbuser->dob_month;
  usr->dob_day = dbuser->dob_day;
  usr->sex = dbuser->sex;
  
  /* Access control */
  usr->priv = dbuser->priv;
  usr->xkeys = dbuser->xkeys;
  
  /* Expiry */
  usr->xp_priv = dbuser->xp_priv;
  usr->xp_date = dbuser->xp_date;
  usr->xp_mins = dbuser->xp_mins;
  usr->xp_flag = dbuser->xp_flag;
  
  /* Statistics */
  usr->times = dbuser->times;
  usr->call = dbuser->call;
  usr->msgs_posted = dbuser->msgs_posted;
  usr->msgs_read = dbuser->msgs_read;
  usr->nup = dbuser->nup;
  usr->ndown = dbuser->ndown;
  usr->ndowntoday = dbuser->ndowntoday;
  usr->up = dbuser->up;
  usr->down = dbuser->down;
  usr->downtoday = dbuser->downtoday;
  
  /* Session info */
  usr->ludate = dbuser->ludate;
  usr->date_1stcall = dbuser->date_1stcall;
  usr->date_pwd_chg = dbuser->date_pwd_chg;
  usr->date_newfile = dbuser->date_newfile;
  usr->time = dbuser->time;
  usr->time_added = dbuser->time_added;
  usr->timeremaining = dbuser->timeremaining;
  
  /* Preferences */
  usr->video = dbuser->video;
  usr->lang = dbuser->lang;
  usr->width = dbuser->width;
  usr->len = dbuser->len;
  usr->help = dbuser->help;
  usr->nulls = dbuser->nulls;
  usr->def_proto = dbuser->def_proto;
  usr->compress = dbuser->compress;
  
  /* Area tracking */
  usr->lastread_ptr = dbuser->lastread_ptr;
  strncpy((char*)usr->msg, dbuser->msg, sizeof(usr->msg) - 1);
  strncpy((char*)usr->files, dbuser->files, sizeof(usr->files) - 1);
  
  /* Credits */
  usr->credit = dbuser->credit;
  usr->debit = dbuser->debit;
  usr->point_credit = dbuser->point_credit;
  usr->point_debit = dbuser->point_debit;
  
  /* Flags (bits already set for BITS_ENCRYPT above) */
  usr->bits |= dbuser->bits;
  usr->bits2 = dbuser->bits2;
  usr->delflag = dbuser->delflag;
  
  /* Misc */
  usr->group = dbuser->group;
  usr->extra = dbuser->extra;
  
  /* Set struct_len field */
  usr->struct_len = sizeof(struct _usr) / 20;
}

dword _fast UserHash(byte *f)
{
  dword hash=0, g;
  signed char *p;

  for (p=f; *p; p++)
  {
    hash=(hash << 4) + (dword)tolower(*p);

    if ((g=(hash & 0xf0000000L)) != 0L)
    {
      hash |= g >> 24;
      hash |= g;
    }
  }
  
  return (hash & 0x7fffffffLu);
}




/* This function will rebuild the user file index if it determines that
 * there is a record count mismatch between the main user file and the
 * index file.
 */

static void near _RebuildIndex(HUF huf)
{
  long idxsize=lseek(huf->fdndx, 0L, SEEK_END) / (long)sizeof(USRNDX);
  long size=UserFileSize(huf);
  struct _usr user;
  USRNDX usrndx;

  /* If the index and user files are the same number of records, there
   * is no need to rebuild.
   */

  if (size==idxsize)
    return;

  lseek(huf->fdbbs, 0L, SEEK_SET);
  lseek(huf->fdndx, 0L, SEEK_SET);

  /* Truncate index file to zero bytes long */

  setfsize(huf->fdndx, 0L);

  lseek(huf->fdbbs, 0L, SEEK_SET);

  while (read(huf->fdbbs, (char *)&user, sizeof user)==sizeof user)
  {
    usrndx.hash_name=UserHash(user.name);
    usrndx.hash_alias=UserHash(user.alias);

    if (write(huf->fdndx, (char *)&usrndx, sizeof usrndx) != sizeof usrndx)
      return;
  }
}


/* Returns TRUE if the usr.name or usr.alias fields match those given
 * in the user structure.
 */

static int near _UserMatch(HUF huf, long ofs, char *name, char *alias,
                           struct _usr *pusr)
{
  long pos=(long)sizeof(struct _usr) * ofs;
  int fNameMatch;
  int fAliasMatch;

  if (lseek(huf->fdbbs, pos, SEEK_SET) != pos)
    return FALSE;

  if (read(huf->fdbbs, (char *)pusr, sizeof *pusr) != sizeof *pusr)
    return FALSE;

  fNameMatch=(name && eqstri(pusr->name, name));
  fAliasMatch=(alias && eqstri(pusr->alias, alias));

  return ((fNameMatch && !alias) ||
          (fAliasMatch && !name) ||
          (fNameMatch && fAliasMatch) ||
          (!name && !alias));
}




/* Open the user file for access.  To create a new user file, specify
 * O_CREAT | O_TRUNC for the mode parameter.
 */

HUF _fast UserFileOpen(char *name, int mode)
{
  char filename[PATHLEN];
  HUF huf;

  if ((huf=malloc(sizeof(*huf)))==NULL)
    return NULL;

  huf->id_huf=ID_HUF;
  huf->db = NULL;
  huf->use_sqlite = _use_sqlite_backend();
  huf->last_found_id = -1;

  {
    int flags = MAXDB_OPEN_READWRITE;

    if (mode & O_CREAT)
      flags |= MAXDB_OPEN_CREATE;

    strcpy(filename, name);
    strcat(filename, ".db");

    huf->db = maxdb_open(filename, flags);
    if (!huf->db)
    {
      free(huf);
      return NULL;
    }

    if (maxdb_schema_upgrade((MaxDB*)huf->db, 1) != MAXDB_OK)
    {
      maxdb_close((MaxDB*)huf->db);
      free(huf);
      return NULL;
    }

    huf->fdbbs = -1;
    huf->fdndx = -1;

    return huf;
  }
}



/* Return the number of users in the user file */

long _fast UserFileSize(HUF huf)
{
  long len;

  if (!huf || huf->id_huf != ID_HUF)
    return -1L;

  /* SQLite backend: report legacy-style size as (max id + 1) */
  if (huf->use_sqlite && huf->db)
  {
    int next_id;
    if (maxdb_user_next_id((MaxDB *)huf->db, &next_id) != MAXDB_OK)
      return -1L;
    return (long)next_id;
  }

  len=lseek(huf->fdbbs, 0L, SEEK_END);
  len /= (long)sizeof(struct _usr);

  return len;
}

int _fast UserFileSeek(HUF huf, long rec, struct _usr *pusr, int sz)
{
  long len;

  /* SQLite backend: treat rec as user id */
  if (huf && huf->id_huf == ID_HUF && huf->use_sqlite && huf->db)
  {
    MaxDBUser *dbuser;
    struct _usr tmp;
    int id;

    if (!pusr || sz <= 0)
      return FALSE;

    if (rec == -1)
      rec = UserFileSize(huf) - 1;

    if (rec < 0)
      return FALSE;

    id = (int)rec;
    dbuser = maxdb_user_find_by_id((MaxDB *)huf->db, id);
    if (!dbuser)
      return FALSE;

    _convert_maxdbuser_to_usr(dbuser, &tmp);
    maxdb_user_free(dbuser);

    if (sz > (int)sizeof(tmp))
      sz = sizeof(tmp);
    memcpy(pusr, &tmp, sz);
    return TRUE;
  }

  if ((len=UserFileSize(huf))==-1L)
    return FALSE;

  if (rec==-1)
    rec=len-1;

  if (rec < 0 || rec >= len)
    return FALSE;

  lseek(huf->fdbbs, rec*sizeof(struct _usr), SEEK_SET);
  return read(huf->fdbbs, (char *)pusr, sz) == sz;
}


/* Find the index of a user within the user file.  Returns the offset
 * if found, or -1L if the user/alias could not be located.  Either
 * or both of 'name' or 'alias' can be NULL.
 *
 * This is an internal function only!  (It has a plOfs option!)
 * Use the external UserFileFind as an external entrypoint.
 */

static int _fast _UserFileFind(HUF huf, char *name, char *alias,
                               struct _usr *pusr, long *plOfs,
                               long lStartOfs, int fForward)
{
  dword hash_name=name ? UserHash(name) : -1L;
  dword hash_alias=alias ? UserHash(alias) : -1L;
  USRNDX *pun, *pu;
  long ofs;
  int got;

  /* Rebuild the index, if necessary */

  _RebuildIndex(huf);

  /* Allocate memory for a block of user index records */

  if ((pun=malloc(sizeof(USRNDX) * UNDX_BLOCK))==NULL)
    return FALSE;

  /* Seek to the beginning of the index file */

  ofs=lStartOfs;

  for (;;)
  {
    long pos;

    if (fForward)
      pos=ofs;
    else
      pos=ofs > (long)UNDX_BLOCK ? ofs-(long)UNDX_BLOCK : 0L;

    lseek(huf->fdndx, pos * (long)sizeof(USRNDX), SEEK_SET);

    if ((got=read(huf->fdndx, (char *)pun,
                  UNDX_BLOCK * sizeof(USRNDX))) < sizeof(USRNDX))
      break;

    if (!fForward)
      ofs=pos+(long)got-1L;

    /* Divide by the length of the structure */

    got /= (int)sizeof(USRNDX);

    for (pu=fForward ? pun : pun+got-1; got--; fForward ? pu++ : pu--)
    {
      if (name && hash_name==pu->hash_name && !alias ||
          alias && hash_alias==pu->hash_alias && !name ||
          hash_name==pu->hash_name && hash_alias==pu->hash_alias ||
          !name && !alias)
      {
        /* Now check the hash against the user name itself */

        if (_UserMatch(huf, ofs, name, alias, pusr))
        {
          free(pun);
          *plOfs=ofs;
          return TRUE;
        }
      }

      if (fForward)
        ofs++;
      else
        ofs--;
    }
  }

  free(pun);
  return FALSE;
}



/* External entrypoint for UserFileFind */

int _fast UserFileFind(HUF huf, char *name, char *alias, struct _usr *pusr)
{
  long ofs;

  if (!huf || huf->id_huf != ID_HUF)
    return FALSE;

  /* SQLite backend */
  if (huf->use_sqlite && huf->db)
  {
    MaxDBUser *dbuser = NULL;
    
    if (name && *name)
      dbuser = maxdb_user_find_by_name((MaxDB*)huf->db, name);
    else if (alias && *alias)
      dbuser = maxdb_user_find_by_alias((MaxDB*)huf->db, alias);
    
    if (dbuser)
    {
      huf->last_found_id = (long)dbuser->id;
      _convert_maxdbuser_to_usr(dbuser, pusr);
      maxdb_user_free(dbuser);
      return TRUE;
    }
    
    return FALSE;
  }

  /* Legacy file backend */
  if (_UserFileFind(huf, name, alias, pusr, &ofs, 0L, TRUE))
  {
    huf->last_found_id = ofs;
    return TRUE;
  }
  return FALSE;
}


/* UserFileFindOpen
 *
 * This function opens a multi-user find session.  The name and
 * alias parameters are treated the same way as they are in the
 * UserFileFind function.
 *
 * If the user is found, the function returns a HUFF handle and
 * the huff->usr structure contains the record of the found user.
 * UserFileFindNext/Prior must be called (with the appropriate
 * arguments) to find the remaining users.
 *
 * If successful, this function returns a non-NULL handle.  If
 * the user was not found, this function returns NULL.
 */

HUFF _fast UserFileFindOpen(HUF huf, char *name, char *alias)
{
  HUFF huff;

  if (!huf || huf->id_huf != ID_HUF)
    return NULL;

  /* SQLite backend */
  if (huf->use_sqlite && huf->db)
  {
    MaxDBUser *dbuser = NULL;

    if ((huff=malloc(sizeof *huff))==NULL)
      return NULL;

    huff->id_huff=ID_HUFF;
    huff->huf=huf;
    huff->lLastUser=-1L;
    huff->ulStartNum=0L;
    huff->cUsers=0;

    if ((huff->pusr=malloc(UBBS_BLOCK * sizeof(struct _usr)))==NULL)
    {
      free(huff);
      return NULL;
    }

    if (name && *name)
      dbuser = maxdb_user_find_by_name((MaxDB *)huf->db, name);
    else if (alias && *alias)
      dbuser = maxdb_user_find_by_alias((MaxDB *)huf->db, alias);

    if (dbuser)
    {
      _convert_maxdbuser_to_usr(dbuser, &huff->usr);
      huff->lLastUser = (long)dbuser->id;
      maxdb_user_free(dbuser);
      return huff;
    }

    if (!name && !alias)
    {
      if (UserFileFindNext(huff, NULL, NULL))
        return huff;
    }

    free(huff->pusr);
    free(huff);
    return NULL;
  }

  if ((huff=malloc(sizeof *huff))==NULL)
    return NULL;

  huff->id_huff=ID_HUFF;
  huff->huf=huf;
  huff->lLastUser=-1L;
  huff->ulStartNum=0L;
  huff->cUsers=0;

  /* Allocate memory for the arary of users */

  if ((huff->pusr=malloc(UBBS_BLOCK * sizeof(struct _usr)))==NULL)
  {
    free(huff);
    return NULL;
  }

  if (!UserFileFindNext(huff, name, alias))
  {
    free(huff->pusr);
    free(huff);
    return NULL;
  }

  return huff;
}


/* Find the next user record in sequence */

int _fast UserFileFindNext(HUFF huff, char *name, char *alias)
{
  HUF huf;
  dword dwSize;
  long ofs;

  if (!huff || huff->id_huff != ID_HUFF)
    return FALSE;

  huf=huff->huf;

  /* SQLite backend */
  if (huf && huf->use_sqlite && huf->db)
  {
    MaxDBUser *dbuser = NULL;

    if (name || alias)
    {
      /* SQLite mode currently treats name/alias search as single-hit */
      if (huff->lLastUser >= 0)
        return FALSE;

      if (name && *name)
        dbuser = maxdb_user_find_by_name((MaxDB *)huf->db, name);
      else if (alias && *alias)
        dbuser = maxdb_user_find_by_alias((MaxDB *)huf->db, alias);

      if (!dbuser)
        return FALSE;

      _convert_maxdbuser_to_usr(dbuser, &huff->usr);
      huff->lLastUser = (long)dbuser->id;
      maxdb_user_free(dbuser);
      return TRUE;
    }

    dbuser = maxdb_user_find_next_after_id((MaxDB *)huf->db, (int)huff->lLastUser);
    if (!dbuser)
    {
      huff->lLastUser = UserFileSize(huf);
      return FALSE;
    }

    _convert_maxdbuser_to_usr(dbuser, &huff->usr);
    huff->lLastUser = (long)dbuser->id;
    maxdb_user_free(dbuser);
    return TRUE;
  }

  dwSize=UserFileSize(huf);

  /* If we're looking for the next instance of a specific name, use the
   * index to do the search.
   */

  if (name || alias)
  {
    if (_UserFileFind(huf, name, alias, &huff->usr,
                      &ofs, huff->lLastUser+1, TRUE))
    {
      huff->lLastUser=ofs;
      return TRUE;
    }

    return FALSE;
  }


  /* Loop through all of the offsets until we find what we're looking for */

  for (ofs=(unsigned long)(huff->lLastUser+1); ofs < dwSize; ofs++)
  {
    /* If the offset is out of bounds... */

    if (ofs < huff->ulStartNum || ofs >= huff->ulStartNum+huff->cUsers)
    {
      int size=UBBS_BLOCK * sizeof(struct _usr);
      int got;

      lseek(huf->fdbbs, ofs * (long)sizeof(struct _usr), SEEK_SET);
      got=read(huf->fdbbs, (char *)huff->pusr, size);

      /* Update buffer pointers, if appropriate */

      if (got >= 0)
        got /= sizeof(struct _usr);

      if (got >= 0)
      {
        huff->ulStartNum=ofs;
        huff->cUsers = got;
      }

      /* If we reached EOF, just ignore it */

      if (got < 0 ||
          ofs < huff->ulStartNum ||
          ofs >= huff->ulStartNum+huff->cUsers)
      {
        huff->lLastUser=UserFileSize(huf);
        return FALSE;
      }
    }

    huff->usr=huff->pusr[ofs - huff->ulStartNum];
    huff->lLastUser=ofs;
    return TRUE;
  }

  huff->lLastUser=UserFileSize(huf);
  return FALSE;
}


/* Find the prior user record in sequence */

int _fast UserFileFindPrior(HUFF huff, char *name, char *alias)
{
  HUF huf;
  long ofs;

  if (!huff || huff->id_huff != ID_HUFF)
    return FALSE;

  huf=huff->huf;

  /* SQLite backend */
  if (huf && huf->use_sqlite && huf->db)
  {
    MaxDBUser *dbuser = NULL;
    int start_id;

    if (name || alias)
    {
      if (huff->lLastUser >= 0)
        return FALSE;

      if (name && *name)
        dbuser = maxdb_user_find_by_name((MaxDB *)huf->db, name);
      else if (alias && *alias)
        dbuser = maxdb_user_find_by_alias((MaxDB *)huf->db, alias);

      if (!dbuser)
        return FALSE;

      _convert_maxdbuser_to_usr(dbuser, &huff->usr);
      huff->lLastUser = (long)dbuser->id;
      maxdb_user_free(dbuser);
      return TRUE;
    }

    if (huff->lLastUser < 0)
      start_id = (int)UserFileSize(huf);
    else
      start_id = (int)huff->lLastUser;

    dbuser = maxdb_user_find_prev_before_id((MaxDB *)huf->db, start_id);
    if (!dbuser)
    {
      huff->lLastUser = 0L;
      return FALSE;
    }

    _convert_maxdbuser_to_usr(dbuser, &huff->usr);
    huff->lLastUser = (long)dbuser->id;
    maxdb_user_free(dbuser);
    return TRUE;
  }

  /* If we're looking for the next instance of a specific name, use the
   * index to do the search.
   */

  if (name || alias)
  {
    if (_UserFileFind(huf, name, alias, &huff->usr,
                      &ofs, huff->lLastUser-1, FALSE))
    {
      huff->lLastUser=ofs;
      return TRUE;
    }

    return FALSE;
  }


  /* Loop through all of the offsets until we find what we're looking for */

  for (ofs=huff->lLastUser-1; ofs >= 0; ofs--)
  {
    /* If the offset is out of bounds... */

    if (ofs < huff->ulStartNum || ofs >= huff->ulStartNum+huff->cUsers)
    {
      int size=UBBS_BLOCK * sizeof(struct _usr);
      int got;

      lseek(huf->fdbbs, ofs * (long)sizeof(struct _usr), SEEK_SET);
      got=read(huf->fdbbs, (char *)huff->pusr, size);

      if (got >= 0)
        got /= sizeof(struct _usr);

      if (got >= 0)
      {
        huff->ulStartNum=ofs;
        huff->cUsers = got;
      }

      /* If we reached EOF, just ignore it */

      if (got < 0 ||
          ofs < huff->ulStartNum ||
          ofs >= huff->ulStartNum+huff->cUsers)
      {
        huff->lLastUser=0L;
        return FALSE;
      }
    }

    huff->usr=huff->pusr[ofs - huff->ulStartNum];
    huff->lLastUser=ofs;
    return TRUE;
  }

  huff->lLastUser=0L;
  return FALSE;
}



/* Close a user file finding session */

int _fast UserFileFindClose(HUFF huff)
{
  if (!huff || huff->id_huff != ID_HUFF)
    return FALSE;

  free(huff->pusr);
  free(huff);

  return TRUE;
}



/* Update an existing user record */

int _fast UserFileUpdate(HUF huf, char *name, char *alias, struct _usr *pusr)
{
  USRNDX usrndx;
  struct _usr usr;
  long ofs;

  if (!huf || huf->id_huf != ID_HUF)
    return FALSE;

  /* SQLite backend */
  if (huf->use_sqlite && huf->db)
  {
    MaxDBUser *dbuser;
    MaxDBUser update_user;
    int result;
    
    /* Find the existing user to get their ID */
    if (name && *name)
      dbuser = maxdb_user_find_by_name((MaxDB*)huf->db, name);
    else if (alias && *alias)
      dbuser = maxdb_user_find_by_alias((MaxDB*)huf->db, alias);
    else
    {
      logit("!UserFileUpdate: no name or alias provided");
      return FALSE;
    }
    
    if (!dbuser)
    {
      logit("!UserFileUpdate: user not found (name='%s' alias='%s')",
            name ? (char*)name : "(null)",
            alias ? (char*)alias : "(null)");
      return FALSE;
    }
    
    /* Convert updated struct _usr to MaxDBUser, preserving ID */
    _convert_usr_to_maxdbuser(pusr, &update_user, dbuser->id);
    maxdb_user_free(dbuser);
    
    /* Update in database */
    result = maxdb_user_update((MaxDB*)huf->db, &update_user);
    if (result != MAXDB_OK)
    {
      logit("!UserFileUpdate: SQLite update failed (rc=%d): %s",
            result, maxdb_error((MaxDB*)huf->db));
    }
    return (result == MAXDB_OK) ? TRUE : FALSE;
  }

  /* Legacy file backend */
  /* Try to find the old user */

  if (!_UserFileFind(huf, name, alias, &usr, &ofs, 0L, TRUE))
    return FALSE;


  /* Seek to appropriate offset */

  lseek(huf->fdbbs, ofs * (long)sizeof(struct _usr), SEEK_SET);
  lseek(huf->fdndx, ofs * (long)sizeof(USRNDX), SEEK_SET);


  /* Construct the new index entry for this user */

  usrndx.hash_name=UserHash(pusr->name);
  usrndx.hash_alias=UserHash(pusr->alias);


  /* Return TRUE only if both writes succeed */

  return write(huf->fdbbs, (char *)pusr, sizeof *pusr) == sizeof *pusr &&
         write(huf->fdndx, (char *)&usrndx, sizeof(USRNDX)) == sizeof(USRNDX);
}

/* Write a new user record to the user filee */

int _fast UserFileCreateRecord(HUF huf, struct _usr *pusr, int fCheckUnique)
{
  USRNDX usrndx;
  long ofs=UserFileSize(huf);
  struct _usr junkusr;

  if (!huf || huf->id_huf != ID_HUF)
    return FALSE;

  /* SQLite backend */
  if (huf->use_sqlite && huf->db)
  {
    MaxDBUser new_user;
    int new_id;
    int result;
    
    /* Check uniqueness if requested */
    if (fCheckUnique && UserFileFind(huf, (char*)pusr->name, NULL, &junkusr))
      return FALSE;
    
    /* Convert struct _usr to MaxDBUser (ID will be auto-assigned) */
    _convert_usr_to_maxdbuser(pusr, &new_user, 0);
    
    /* Create in database */
    result = maxdb_user_create((MaxDB*)huf->db, &new_user, &new_id);
    return (result == MAXDB_OK) ? TRUE : FALSE;
  }

  /* Legacy file backend */
  /* Make sure that the user doesn't already exist */

  if (fCheckUnique && UserFileFind(huf, (char*)pusr->name, NULL, &junkusr))
    return FALSE;

  /* Append to end */

  lseek(huf->fdbbs, ofs * (long)sizeof(struct _usr), SEEK_SET);
  lseek(huf->fdndx, ofs * (long)sizeof(USRNDX), SEEK_SET);

  /* Construct the new index entry for this user */

  usrndx.hash_name=UserHash(pusr->name);
  usrndx.hash_alias=UserHash(pusr->alias);

  /* Return TRUE only if both writes succeed */

  return write(huf->fdbbs, (char *)pusr, sizeof *pusr) == sizeof *pusr &&
         write(huf->fdndx, (char *)&usrndx, sizeof(USRNDX)) == sizeof(USRNDX);
}

/* Not supported - user file must be packed to delete user */

int _fast UserFileRemove(HUF huf, struct _usr *pusr)
{
  NW(huf);
  NW(pusr);

  return FALSE;
}


/* Close the user file */

int _fast UserFileClose(HUF huf)
{
  if (!huf || huf->id_huf != ID_HUF)
    return FALSE;

  /* SQLite backend */
  if (huf->use_sqlite && huf->db)
  {
    maxdb_close((MaxDB*)huf->db);
    free(huf);
    return TRUE;
  }

  /* Legacy file backend */
  if (huf->fdbbs != -1)
    close(huf->fdbbs);

  if (huf->fdndx != -1)
    close(huf->fdndx);

  memset(huf, 0, sizeof huf);
  free(huf);
  return TRUE;
}


/**
 * @brief Return the record id/offset of the last user found by UserFileFind().
 * @return Record id (>= 0) or -1 if no find has been performed.
 */
long _fast UserFileGetLastFoundId(HUF huf)
{
  if (!huf || huf->id_huf != ID_HUF)
    return -1;

  return huf->last_found_id;
}
