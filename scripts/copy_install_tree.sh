#! /bin/sh
#
# copy_install_tree.sh - Copy install_tree to PREFIX
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

[ -d "${PREFIX}/docs}" ] || mkdir -p "${PREFIX}/docs"

if [ -f "${PREFIX}/etc/max.ctl" ] && [ "$FORCE" = "0" ]; then
  echo "This is not a fresh install -- not copying install tree.."
  echo "Use --force to overwrite (creates backups of existing files)"
else
  # Backup existing if --force
  if [ "$FORCE" = "1" ] && [ -d "${PREFIX}/etc" ]; then
    BACKUP="${PREFIX}/backup-$(date +%Y%m%d-%H%M%S)"
    echo "Backing up existing installation to ${BACKUP}..."
    mkdir -p "${BACKUP}"
    [ -d "${PREFIX}/etc" ] && cp -rp "${PREFIX}/etc" "${BACKUP}/"
    [ -d "${PREFIX}/m" ] && cp -rp "${PREFIX}/m" "${BACKUP}/"
  fi

  echo "Copying install tree to ${PREFIX}.."
  cp -rp install_tree/* "${PREFIX}"

  if [ "${PREFIX}" != "/var/max" ]; then
    echo "Modifying configuration files to reflect PREFIX=${PREFIX}.."
    for file in etc/max.ctl etc/areas.bbs etc/compress.cfg etc/squish.cfg etc/sqafix.cfg
    do
      echo " - ${file}"
      LC_ALL=C cat "${PREFIX}/${file}" | LC_ALL=C sed "s;/var/max;${PREFIX};g" > "${PREFIX}/${file}.tmp"
      mv -f "${PREFIX}/${file}.tmp" "${PREFIX}/${file}"
    done
  fi
fi

if [ -f "${PREFIX}/bin/runbbs.sh" ] && [ "$FORCE" = "0" ]; then
  echo "This is not a fresh install -- not copying runbbs.sh.."
else
  cp scripts/runbbs.sh "${PREFIX}/bin/runbbs.sh"
fi

cp docs/max_mast.txt "${PREFIX}/docs"

exit 0
