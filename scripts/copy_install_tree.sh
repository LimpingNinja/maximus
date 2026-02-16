#! /bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
#
# copy_install_tree.sh - Copy install_tree to PREFIX
#
# Copyright (C) 2003 Wes Garland
# Copyright (C) 2025 Kevin Morgan (Limping Ninja) - https://github.com/LimpingNinja
#
# Usage: copy_install_tree.sh [PREFIX] [--force]
#   --force: Overwrite existing files (creates backups)
#

FORCE=0
for arg in "$@"; do
  case "$arg" in
    --force) FORCE=1 ;;
    -*) echo "Unknown option: $arg"; exit 1 ;;
    *) PREFIX="$arg" ;;
  esac
done

if [ ! "${PREFIX}" ]; then
  echo "Error: PREFIX not set!"
  echo "Usage: $0 <prefix> [--force]"
  exit 1
fi

export PREFIX

[ -d "${PREFIX}/docs" ] || mkdir -p "${PREFIX}/docs"

if [ -f "${PREFIX}/config/maximus.toml" ] && [ "$FORCE" = "0" ]; then
  echo "This is not a fresh install -- not copying install tree.."
  echo "Use --force to overwrite (creates backups of existing files)"
else
  # Backup existing if --force
  if [ "$FORCE" = "1" ] && [ -d "${PREFIX}/config" ]; then
    BACKUP="${PREFIX}/backup-$(date +%Y%m%d-%H%M%S)"
    echo "Backing up existing installation to ${BACKUP}..."
    mkdir -p "${BACKUP}"
    [ -d "${PREFIX}/config" ] && cp -rp "${PREFIX}/config" "${BACKUP}/"
    [ -d "${PREFIX}/scripts" ] && cp -rp "${PREFIX}/scripts" "${BACKUP}/"
    [ -d "${PREFIX}/display" ] && cp -rp "${PREFIX}/display" "${BACKUP}/"
  fi

  echo "Copying install tree to ${PREFIX}.."
  cp -rp resources/install_tree/* "${PREFIX}"

  if [ "${PREFIX}" != "/var/max" ]; then
    echo "Modifying configuration files to reflect PREFIX=${PREFIX}.."
    for file in config/legacy/max.ctl config/legacy/areas.bbs config/compress.cfg config/squish.cfg config/sqafix.cfg
    do
      if [ -f "${PREFIX}/${file}" ]; then
        echo " - ${file}"
        LC_ALL=C cat "${PREFIX}/${file}" | LC_ALL=C sed "s;/var/max;${PREFIX};g" > "${PREFIX}/${file}.tmp"
        mv -f "${PREFIX}/${file}.tmp" "${PREFIX}/${file}"
      fi
    done
  fi
fi

if [ -f "${PREFIX}/runbbs.sh" ] && [ "$FORCE" = "0" ]; then
  echo "This is not a fresh install -- not copying runbbs.sh.."
else
  cp build/runbbs.sh "${PREFIX}/runbbs.sh" 2>/dev/null || true
fi

cp docs/max_mast.txt "${PREFIX}/docs"

exit 0
