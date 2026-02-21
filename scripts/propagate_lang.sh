#!/usr/bin/env bash
# scripts/propagate_lang.sh
#
# Propagates language files from resources/lang to install_tree and build.
#
# Workflow:
#   1. Copy stock english.toml + delta_english.toml → install_tree/config/lang/
#   2. Apply delta (full merge) to the install_tree copy
#   3. Copy merged english.toml + delta → build/config/lang/
#
# Usage: ./scripts/propagate_lang.sh
#   Run from the repository root or any subdirectory.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAXCFG="$REPO_ROOT/build/bin/maxcfg"

STOCK="$REPO_ROOT/resources/lang/english.toml"
DELTA="$REPO_ROOT/resources/lang/delta_english.toml"
INSTALL_LANG="$REPO_ROOT/resources/install_tree/config/lang"
BUILD_LANG="$REPO_ROOT/build/config/lang"

if [ ! -x "$MAXCFG" ]; then
    echo "ERROR: maxcfg binary not found at $MAXCFG" >&2
    echo "       Run 'make build' first." >&2
    exit 1
fi

echo "→ Copying stock + delta to install_tree..."
cp "$STOCK" "$INSTALL_LANG/english.toml"
cp "$DELTA"  "$INSTALL_LANG/delta_english.toml"

echo "→ Applying delta (full merge) to install_tree copy..."
"$MAXCFG" --apply-delta "$INSTALL_LANG/english.toml" --delta "$DELTA" --full

echo "→ Copying merged + delta to build/config/lang..."
cp "$INSTALL_LANG/english.toml" "$BUILD_LANG/english.toml"
cp "$DELTA"                      "$BUILD_LANG/delta_english.toml"

echo "Done."
