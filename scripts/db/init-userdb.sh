#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# init-userdb.sh - Initialize a starter SQLite user database for Maximus
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# Usage:
#   scripts/db/init-userdb.sh <prefix>
#   scripts/db/init-userdb.sh <prefix> <db_path>
#
# Notes:
# - This script is intended to be invoked by the build/install flow once the
#   SQLite-backed user DB exists (Milestone 1).
# - Preferred path is to run the compiled init_userdb helper (no sqlite3 CLI needed).
# - Falls back to sqlite3 CLI if the helper is not present.

set -euo pipefail

PREFIX="${1:-}"
DB_PATH="${2:-}"

if [ -z "$PREFIX" ]; then
  echo "Usage: $0 <prefix> [db_path]" >&2
  exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCHEMA_SQL="${SCRIPT_DIR}/userdb_schema.sql"

if [ -f "${PREFIX}/etc/db/userdb_schema.sql" ]; then
  SCHEMA_SQL="${PREFIX}/etc/db/userdb_schema.sql"
fi

if [ -z "$DB_PATH" ]; then
  DB_PATH="${PREFIX}/etc/user.db"
fi

if [ ! -f "$SCHEMA_SQL" ]; then
  echo "Schema not found: $SCHEMA_SQL" >&2
  exit 2
fi

if [ -x "${PREFIX}/bin/init_userdb" ]; then
  exec "${PREFIX}/bin/init_userdb" --prefix "${PREFIX}" --db "${DB_PATH}" --schema "${SCHEMA_SQL}"
fi

if ! command -v sqlite3 >/dev/null 2>&1; then
  echo "init_userdb helper not found at ${PREFIX}/bin/init_userdb and sqlite3 CLI not found in PATH." >&2
  exit 2
fi

mkdir -p "$(dirname "$DB_PATH")"

if [ -f "$DB_PATH" ]; then
  echo "User DB already exists: $DB_PATH"
  exit 0
fi

echo "Initializing user DB: $DB_PATH"
sqlite3 "$DB_PATH" < "$SCHEMA_SQL"
echo "Done."
