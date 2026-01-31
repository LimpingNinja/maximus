#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "compiler.h"
#include "max_u.h"
#include "userapi.h"
#include "libmaxdb.h"
#include "libmaxcfg.h"

static void usage(const char *argv0)
{
  fprintf(stderr,
          "Usage: %s [--src <userfile_root>] [--dst <sqlite_db_path>]\n"
          "\n"
          "  --src  Path root for legacy user files (defaults to maximus.file_password)\n"
          "  --dst  Destination SQLite DB path (defaults to <userfile_root>.db)\n"
          "  --no-verify-lookups  Skip name/alias lookup verification pass\n",
          argv0);
}

static const char *resolve_install_path(const char *argv0, char *out, size_t out_sz)
{
  const char *s = getenv("MAX_INSTALL_PATH");
  if (!s || !*s)
    s = getenv("MAXIMUS");
  if (s && *s)
    return s;

  if (argv0 && *argv0 && out && out_sz > 0)
  {
    char exe_path[1024];
    if (realpath(argv0, exe_path) != NULL)
    {
      char tmp1[1024];
      char tmp2[1024];
      const char *bin_dir;
      const char *prefix_dir;

      strncpy(tmp1, exe_path, sizeof(tmp1) - 1);
      tmp1[sizeof(tmp1) - 1] = '\0';
      bin_dir = dirname(tmp1);
      if (bin_dir && *bin_dir)
      {
        strncpy(tmp2, bin_dir, sizeof(tmp2) - 1);
        tmp2[sizeof(tmp2) - 1] = '\0';
        prefix_dir = dirname(tmp2);
        if (prefix_dir && *prefix_dir)
        {
          if (snprintf(out, out_sz, "%s", prefix_dir) < (int)out_sz)
            return out;
        }
      }
    }
  }

  return ".";
}

static int path_is_absolute(const char *path)
{
  if (!path || !*path)
    return 0;
  if (path[0] == '/' || path[0] == '\\')
    return 1;
  if (isalpha((unsigned char)path[0]) && path[1] == ':')
    return 1;
  return 0;
}

static char *resolve_userfile_root(const char *sys_path, const char *raw)
{
  const char *p;
  size_t len;
  char *tmp;

  if (!raw || !*raw)
    return NULL;

  p = raw;
  if (p[0] == ':')
    p++;

  len = strlen(p);
  if (len >= 4)
  {
    const char *ext = p + len - 4;
    if (strcasecmp(ext, ".bbs") == 0 || strcasecmp(ext, ".idx") == 0)
      len -= 4;
  }

  tmp = (char *)malloc(len + 1);
  if (!tmp)
    return NULL;
  memcpy(tmp, p, len);
  tmp[len] = '\0';

  if (path_is_absolute(tmp))
    return tmp;

  if (!sys_path || !*sys_path)
    return tmp;

  {
    size_t sys_len = strlen(sys_path);
    while (sys_len > 1 && (sys_path[sys_len - 1] == '/' || sys_path[sys_len - 1] == '\\'))
      sys_len--;

    {
      size_t out_len = sys_len + 1 + strlen(tmp) + 1;
      char *out = (char *)malloc(out_len);
      if (!out)
        return tmp;
      memcpy(out, sys_path, sys_len);
      out[sys_len] = '/';
      memcpy(out + sys_len + 1, tmp, strlen(tmp) + 1);
      free(tmp);
      return out;
    }
  }
}

static int load_userfile_root_from_config(const char *sys_path, char **out_root)
{
  char maximus_path[1024];
  MaxCfgToml *cfg = NULL;
  MaxCfgVar v;
  MaxCfgStatus st;

  if (!out_root)
    return 0;
  *out_root = NULL;

  if (maxcfg_resolve_path(sys_path, "config/maximus", maximus_path, sizeof(maximus_path)) != MAXCFG_OK)
    return 0;

  st = maxcfg_toml_init(&cfg);
  if (st != MAXCFG_OK || cfg == NULL)
    return 0;

  st = maxcfg_toml_load_file(cfg, maximus_path, "maximus");
  if (st != MAXCFG_OK)
  {
    maxcfg_toml_free(cfg);
    return 0;
  }

  if (maxcfg_toml_get(cfg, "maximus.file_password", &v) == MAXCFG_OK &&
      v.type == MAXCFG_VAR_STRING && v.v.s && *v.v.s)
  {
    *out_root = resolve_userfile_root(sys_path, v.v.s);
  }

  maxcfg_toml_free(cfg);
  return (*out_root != NULL);
}

static int file_exists(const char *path)
{
  struct stat st;
  return (path && *path && stat(path, &st) == 0);
}

static void usr_to_dbuser(const struct _usr *usr, MaxDBUser *dbuser, int id)
{
  memset(dbuser, 0, sizeof(MaxDBUser));
  dbuser->id = id;

  strncpy(dbuser->name, (const char *)usr->name, sizeof(dbuser->name) - 1);
  strncpy(dbuser->city, (const char *)usr->city, sizeof(dbuser->city) - 1);
  strncpy(dbuser->alias, (const char *)usr->alias, sizeof(dbuser->alias) - 1);
  strncpy(dbuser->phone, (const char *)usr->phone, sizeof(dbuser->phone) - 1);
  strncpy(dbuser->dataphone, (const char *)usr->dataphone, sizeof(dbuser->dataphone) - 1);

  memcpy(dbuser->pwd, usr->pwd, sizeof(dbuser->pwd));
  dbuser->pwd_encrypted = (usr->bits & BITS_ENCRYPT) ? 1 : 0;

  dbuser->dob_year = usr->dob_year;
  dbuser->dob_month = usr->dob_month;
  dbuser->dob_day = usr->dob_day;
  dbuser->sex = usr->sex;

  dbuser->priv = usr->priv;
  dbuser->xkeys = usr->xkeys;

  dbuser->xp_priv = usr->xp_priv;
  dbuser->xp_date = usr->xp_date;
  dbuser->xp_mins = usr->xp_mins;
  dbuser->xp_flag = usr->xp_flag;

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

  dbuser->ludate = usr->ludate;
  dbuser->date_1stcall = usr->date_1stcall;
  dbuser->date_pwd_chg = usr->date_pwd_chg;
  dbuser->date_newfile = usr->date_newfile;
  dbuser->time = usr->time;
  dbuser->time_added = usr->time_added;
  dbuser->timeremaining = usr->timeremaining;

  dbuser->video = usr->video;
  dbuser->lang = usr->lang;
  dbuser->width = usr->width;
  dbuser->len = usr->len;
  dbuser->help = usr->help;
  dbuser->nulls = usr->nulls;
  dbuser->def_proto = usr->def_proto;
  dbuser->compress = usr->compress;

  dbuser->lastread_ptr = usr->lastread_ptr;
  strncpy(dbuser->msg, (const char *)usr->msg, sizeof(dbuser->msg) - 1);
  strncpy(dbuser->files, (const char *)usr->files, sizeof(dbuser->files) - 1);

  dbuser->credit = usr->credit;
  dbuser->debit = usr->debit;
  dbuser->point_credit = usr->point_credit;
  dbuser->point_debit = usr->point_debit;

  dbuser->bits = usr->bits;
  dbuser->bits2 = usr->bits2;
  dbuser->delflag = usr->delflag;

  dbuser->group = usr->group;
  dbuser->extra = usr->extra;
}

static void usr_field_to_cstr(const unsigned char *src, size_t src_len,
                               char *dst, size_t dst_size)
{
  size_t n;

  if (!dst || dst_size == 0)
    return;

  dst[0] = '\0';
  if (!src || src_len == 0)
    return;

  n = src_len;
  if (n >= dst_size)
    n = dst_size - 1;

  memcpy(dst, src, n);
  dst[n] = '\0';
}

int main(int argc, char **argv)
{
  const char *src_root = NULL;
  const char *dst_db = NULL;
  int verify_lookups = 1;
  int i;

  char *src_root_owned = NULL;
  char dst_default[1024];
  const char *install_path;
  char install_path_buf[1024];

  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--src") == 0)
    {
      if (i + 1 >= argc)
      {
        usage(argv[0]);
        return 2;
      }
      src_root = argv[++i];
    }
    else if (strcmp(argv[i], "--dst") == 0)
    {
      if (i + 1 >= argc)
      {
        usage(argv[0]);
        return 2;
      }
      dst_db = argv[++i];
    }
    else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
    {
      usage(argv[0]);
      return 0;
    }
    else if (strcmp(argv[i], "--no-verify-lookups") == 0)
    {
      verify_lookups = 0;
    }
    else
    {
      usage(argv[0]);
      return 2;
    }
  }

  install_path = resolve_install_path(argv[0], install_path_buf, sizeof(install_path_buf));
  if (install_path && strcmp(install_path, ".") != 0)
    (void)chdir(install_path);

  if (src_root && *src_root)
  {
    src_root_owned = resolve_userfile_root(".", src_root);
    src_root = src_root_owned;
  }
  else
  {
    if (!load_userfile_root_from_config(".", &src_root_owned))
    {
      fprintf(stderr, "Unable to determine user file root; set maximus.file_password or pass --src\n");
      return 2;
    }
    src_root = src_root_owned;
  }

  if (!dst_db || !*dst_db)
  {
    if (snprintf(dst_default, sizeof(dst_default), "%s.db", src_root) >= (int)sizeof(dst_default))
    {
      fprintf(stderr, "Destination DB path too long\n");
      free(src_root_owned);
      return 2;
    }
    dst_db = dst_default;
  }

  if (file_exists(dst_db))
  {
    fprintf(stderr, "Refusing to overwrite existing DB: %s\n", dst_db);
    free(src_root_owned);
    return 2;
  }

  {
    int fd = -1;
    char src_bbs[1024];
    long legacy_count = 0;
    long imported = 0;
    long rec;
    MaxDB *db = NULL;
    int tx_open = 0;
    int exit_code = 0;

    {
      struct stat st;
      if (snprintf(src_bbs, sizeof(src_bbs), "%s.bbs", src_root) >= (int)sizeof(src_bbs))
      {
        fprintf(stderr, "Source user.bbs path too long\n");
        exit_code = 1;
        goto done;
      }

      fd = open(src_bbs, O_RDONLY | O_BINARY);
      if (fd < 0)
      {
        fprintf(stderr, "Failed to open legacy user file: %s\n", src_bbs);
        exit_code = 1;
        goto done;
      }

      if (fstat(fd, &st) != 0)
      {
        fprintf(stderr, "Failed to stat legacy user file: %s\n", src_bbs);
        exit_code = 1;
        goto done;
      }

      if (st.st_size <= 0 || (st.st_size % (off_t)sizeof(struct _usr)) != 0)
      {
        fprintf(stderr, "Legacy user file size is invalid: %s\n", src_bbs);
        exit_code = 1;
        goto done;
      }

      legacy_count = (long)(st.st_size / (off_t)sizeof(struct _usr));
    }

    db = maxdb_open(dst_db, MAXDB_OPEN_READWRITE | MAXDB_OPEN_CREATE);
    if (!db)
    {
      fprintf(stderr, "maxdb_open failed for %s\n", dst_db);
      exit_code = 1;
      goto done;
    }

    if (maxdb_schema_upgrade(db, 1) != MAXDB_OK)
    {
      fprintf(stderr, "schema upgrade failed: %s\n", maxdb_error(db));
      exit_code = 1;
      goto done;
    }

    if (maxdb_begin_transaction(db) != MAXDB_OK)
    {
      fprintf(stderr, "BEGIN failed: %s\n", maxdb_error(db));
      exit_code = 1;
      goto done;
    }
    tx_open = 1;

    for (rec = 0; rec < legacy_count; rec++)
    {
      struct _usr usr;
      MaxDBUser dbuser;
      int rc;

      if (lseek(fd, (off_t)rec * (off_t)sizeof(struct _usr), SEEK_SET) == (off_t)-1)
      {
        fprintf(stderr, "lseek failed at record %ld\n", rec);
        exit_code = 1;
        goto done;
      }

      if (read(fd, (char *)&usr, sizeof(usr)) != (int)sizeof(usr))
      {
        fprintf(stderr, "read failed at record %ld\n", rec);
        exit_code = 1;
        goto done;
      }

      usr_to_dbuser(&usr, &dbuser, (int)rec);
      rc = maxdb_user_create_with_id(db, &dbuser);
      if (rc != MAXDB_OK)
      {
        fprintf(stderr, "insert failed at legacy id %ld (name=%s): %s\n",
                rec, (char *)usr.name, maxdb_error(db));
        exit_code = 1;
        goto done;
      }

      imported++;
    }

    if (verify_lookups)
    {
      long name_checked = 0;
      long alias_checked = 0;
      long name_failed = 0;
      long alias_failed = 0;
      long alias_warn = 0;

      for (rec = 0; rec < legacy_count; rec++)
      {
        struct _usr usr;
        char namebuf[sizeof(usr.name) + 1];
        char aliasbuf[sizeof(usr.alias) + 1];
        MaxDBUser *found;

        if (lseek(fd, (off_t)rec * (off_t)sizeof(struct _usr), SEEK_SET) == (off_t)-1)
        {
          fprintf(stderr, "lseek failed during verification at record %ld\n", rec);
          exit_code = 1;
          goto done;
        }

        if (read(fd, (char *)&usr, sizeof(usr)) != (int)sizeof(usr))
        {
          fprintf(stderr, "read failed during verification at record %ld\n", rec);
          exit_code = 1;
          goto done;
        }

        usr_field_to_cstr((const unsigned char *)usr.name, sizeof(usr.name),
                          namebuf, sizeof(namebuf));
        usr_field_to_cstr((const unsigned char *)usr.alias, sizeof(usr.alias),
                          aliasbuf, sizeof(aliasbuf));

        if (namebuf[0])
        {
          found = maxdb_user_find_by_name(db, namebuf);
          name_checked++;
          if (!found)
          {
            fprintf(stderr, "verify(name): not found for legacy id %ld (name=%s)\n", rec, namebuf);
            name_failed++;
          }
          else
          {
            if (found->id != (int)rec)
            {
              fprintf(stderr,
                      "verify(name): mismatch for name=%s legacy id %ld -> db id %d\n",
                      namebuf, rec, found->id);
              name_failed++;
            }
            maxdb_user_free(found);
          }
        }

        if (aliasbuf[0])
        {
          found = maxdb_user_find_by_alias(db, aliasbuf);
          alias_checked++;
          if (!found)
          {
            fprintf(stderr, "verify(alias): not found for legacy id %ld (alias=%s)\n", rec, aliasbuf);
            alias_failed++;
          }
          else
          {
            if (found->id != (int)rec)
              alias_warn++;

            maxdb_user_free(found);
          }
        }
      }

      if (name_failed || alias_failed)
      {
        fprintf(stderr,
                "Lookup verification failed: name_checked=%ld name_failed=%ld alias_checked=%ld alias_failed=%ld alias_warn=%ld\n",
                name_checked, name_failed, alias_checked, alias_failed, alias_warn);
        exit_code = 1;
        goto done;
      }

      if (alias_warn)
      {
        fprintf(stderr,
                "WARNING: alias verification returned a different id %ld times (aliases may be non-unique)\n",
                alias_warn);
      }
    }

    if (maxdb_commit(db) != MAXDB_OK)
    {
      fprintf(stderr, "COMMIT failed: %s\n", maxdb_error(db));
      exit_code = 1;
      goto done;
    }
    tx_open = 0;

    {
      int db_count = maxdb_user_count(db);
      if (db_count != (int)legacy_count)
        fprintf(stderr, "WARNING: legacy count=%ld but DB reports %d\n", legacy_count, db_count);
    }

    fprintf(stdout, "Imported %ld/%ld users from %s into %s\n", imported, legacy_count, src_root, dst_db);

done:
    if (db)
    {
      if (tx_open)
        (void)maxdb_rollback(db);
      maxdb_close(db);
    }
    if (fd >= 0)
      close(fd);

    free(src_root_owned);
    return exit_code;
  }
}
