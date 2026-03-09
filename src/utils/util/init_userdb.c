/*
 * init_userdb.c — SQLite user database initializer
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

#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <sqlite3.h>

/**
 * @brief Determine the BBS install root from environment or argv[0].
 *
 * @param argv0   Program path from argv[0]
 * @param out     Output buffer for resolved path
 * @param out_sz  Size of the output buffer
 * @return Pointer to the resolved install path string
 */
static const char *resolve_install_path(const char *argv0, char *out, size_t out_sz)
{
  const char *s = getenv("MAX_INSTALL_PATH");
  if (!s || !*s)
    s = getenv("MAXIMUS");
  if (s && *s)
    return s;

  if (argv0 && *argv0 && out && out_sz > 0)
  {
    char exe_path[PATH_MAX];
    if (realpath(argv0, exe_path) != NULL)
    {
      char tmp1[PATH_MAX];
      char tmp2[PATH_MAX];
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

/**
 * @brief Print usage information to stderr.
 *
 * @param argv0  Program name from argv[0]
 */
static void usage(const char *argv0)
{
  fprintf(stderr,
          "Usage: %s [--prefix <prefix>] [--db <db_path>] [--schema <schema_sql>]\n",
          argv0);
}

/**
 * @brief Check if a file exists.
 *
 * @param path  Path to check
 * @return 1 if exists, 0 otherwise
 */
static int file_exists(const char *path)
{
  struct stat st;
  return stat(path, &st) == 0;
}

/**
 * @brief Recursively create directories (like mkdir -p).
 *
 * @param dir  Directory path to create
 * @return 0 on success, -1 on failure
 */
static int mkdir_p(const char *dir)
{
  char *tmp;
  char *p;

  if (!dir || !*dir)
    return -1;

  tmp = strdup(dir);
  if (!tmp)
    return -1;

  for (p = tmp + 1; *p; p++)
  {
    if (*p == '/')
    {
      *p = '\0';
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
      {
        free(tmp);
        return -1;
      }
      *p = '/';
    }
  }

  if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
  {
    free(tmp);
    return -1;
  }

  free(tmp);
  return 0;
}

/**
 * @brief Ensure the parent directory of a file path exists.
 *
 * @param path  File path whose parent directory should be created
 * @return 0 on success, -1 on failure
 */
static int ensure_parent_dir(const char *path)
{
  char *copy;
  char *slash;

  copy = strdup(path);
  if (!copy)
    return -1;

  slash = strrchr(copy, '/');
  if (!slash)
  {
    free(copy);
    return 0;
  }

  if (slash == copy)
  {
    free(copy);
    return 0;
  }

  *slash = '\0';
  if (mkdir_p(copy) != 0)
  {
    free(copy);
    return -1;
  }

  free(copy);
  return 0;
}

/**
 * @brief Read an entire file into a malloc'd buffer.
 *
 * @param path     Path to the file
 * @param out_len  Output: file size in bytes (may be NULL)
 * @return Allocated buffer with file contents (NUL-terminated), or NULL
 */
static char *read_entire_file(const char *path, size_t *out_len)
{
  FILE *fp;
  long sz;
  size_t rd;
  char *buf;

  fp = fopen(path, "rb");
  if (!fp)
    return NULL;

  if (fseek(fp, 0, SEEK_END) != 0)
  {
    fclose(fp);
    return NULL;
  }

  sz = ftell(fp);
  if (sz < 0)
  {
    fclose(fp);
    return NULL;
  }

  if (fseek(fp, 0, SEEK_SET) != 0)
  {
    fclose(fp);
    return NULL;
  }

  buf = (char *)malloc((size_t)sz + 1);
  if (!buf)
  {
    fclose(fp);
    return NULL;
  }

  rd = fread(buf, 1, (size_t)sz, fp);
  fclose(fp);

  if (rd != (size_t)sz)
  {
    free(buf);
    return NULL;
  }

  buf[sz] = '\0';
  if (out_len)
    *out_len = (size_t)sz;
  return buf;
}

/**
 * @brief Entry point for the SQLite user database initializer.
 *
 * Creates a new user database from the schema SQL file.
 *
 * @param argc  Argument count
 * @param argv  Argument vector
 * @return 0 on success, 1 on error, 2 on usage error
 */
int main(int argc, char **argv)
{
  const char *prefix = NULL;
  const char *db_path = NULL;
  const char *schema_path = NULL;
  char default_db[4096];
  char default_schema[4096];
  char prefix_buf[1024];
  int i;

  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--prefix") == 0)
    {
      if (i + 1 >= argc)
      {
        usage(argv[0]);
        return 2;
      }
      prefix = argv[++i];
    }
    else if (strcmp(argv[i], "--db") == 0)
    {
      if (i + 1 >= argc)
      {
        usage(argv[0]);
        return 2;
      }
      db_path = argv[++i];
    }
    else if (strcmp(argv[i], "--schema") == 0)
    {
      if (i + 1 >= argc)
      {
        usage(argv[0]);
        return 2;
      }
      schema_path = argv[++i];
    }
    else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
    {
      usage(argv[0]);
      return 0;
    }
    else
    {
      usage(argv[0]);
      return 2;
    }
  }

  if (!prefix || !*prefix)
    prefix = resolve_install_path(argv[0], prefix_buf, sizeof(prefix_buf));

  if (prefix && strcmp(prefix, ".") != 0)
    (void)chdir(prefix);

  if (!db_path)
  {
    snprintf(default_db, sizeof(default_db), "data/users/user.db");
    db_path = default_db;
  }

  if (!schema_path)
  {
    snprintf(default_schema, sizeof(default_schema), "data/db/userdb_schema.sql");
    schema_path = default_schema;
  }

  if (!file_exists(schema_path))
  {
    fprintf(stderr, "Schema not found: %s\n", schema_path);
    return 2;
  }

  if (file_exists(db_path))
  {
    fprintf(stdout, "User DB already exists: %s\n", db_path);
    return 0;
  }

  if (ensure_parent_dir(db_path) != 0)
  {
    fprintf(stderr, "Failed to create parent directory for: %s\n", db_path);
    return 1;
  }

  {
    sqlite3 *db = NULL;
    char *errmsg = NULL;
    char *schema_sql;

    schema_sql = read_entire_file(schema_path, NULL);
    if (!schema_sql)
    {
      fprintf(stderr, "Failed to read schema: %s\n", schema_path);
      return 1;
    }

    if (sqlite3_open(db_path, &db) != SQLITE_OK)
    {
      fprintf(stderr, "sqlite3_open failed for %s: %s\n", db_path, sqlite3_errmsg(db));
      sqlite3_close(db);
      free(schema_sql);
      return 1;
    }

    if (sqlite3_exec(db, schema_sql, NULL, NULL, &errmsg) != SQLITE_OK)
    {
      fprintf(stderr, "Schema apply failed: %s\n", errmsg ? errmsg : "unknown error");
      sqlite3_free(errmsg);
      sqlite3_close(db);
      free(schema_sql);
      unlink(db_path);
      return 1;
    }

    sqlite3_close(db);
    free(schema_sql);
  }

  fprintf(stdout, "Initialized user DB: %s\n", db_path);
  return 0;
}
