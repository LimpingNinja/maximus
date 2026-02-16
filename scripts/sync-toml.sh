#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# sync-toml.sh - Sync TOML configs between build/ and resources/install_tree/
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# Syncs TOML configuration files from the live build tree into the
# resources/install_tree so that the repo stays current with runtime edits.
# Also syncs install.sh from build/bin/ to resources/install_tree/bin/.
#
# Usage:
#   scripts/sync-toml.sh              # build → install_tree (default)
#   scripts/sync-toml.sh --reverse    # install_tree → build
#   scripts/sync-toml.sh --diff       # show differences only
#

set -euo pipefail

# ── Resolve project root ─────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

BUILD_CONFIG="$PROJECT_ROOT/build/config"
TREE_CONFIG="$PROJECT_ROOT/resources/install_tree/config"
BUILD_BIN="$PROJECT_ROOT/build/bin"
TREE_BIN="$PROJECT_ROOT/resources/install_tree/bin"

# ── Colors ────────────────────────────────────────────────────────────────────
CYAN='\033[0;36m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
WHITE='\033[1;37m'
NC='\033[0m'

log_ok()   { echo -e "  ${GREEN}[SYNC]${NC}  $1"; }
log_new()  { echo -e "  ${CYAN}[ NEW]${NC}  $1"; }
log_skip() { echo -e "  ${YELLOW}[SKIP]${NC}  $1"; }
log_diff() { echo -e "  ${RED}[DIFF]${NC}  $1"; }

# ── Mode parsing ─────────────────────────────────────────────────────────────
MODE="forward"   # build → install_tree
while [ $# -gt 0 ]; do
    case "$1" in
        --reverse|-r)  MODE="reverse"; shift ;;
        --diff|-d)     MODE="diff"; shift ;;
        --help|-h)
            echo "Usage: $(basename "$0") [--reverse | --diff | --help]"
            echo ""
            echo "  (default)    Sync build/config → resources/install_tree/config"
            echo "  --reverse    Sync resources/install_tree/config → build/config"
            echo "  --diff       Show differences only, no changes"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option:${NC} $1"
            exit 1
            ;;
    esac
done

# ── Set source/dest based on mode ────────────────────────────────────────────
if [ "$MODE" = "reverse" ]; then
    SRC_CONFIG="$TREE_CONFIG"
    DST_CONFIG="$BUILD_CONFIG"
    SRC_BIN="$TREE_BIN"
    DST_BIN="$BUILD_BIN"
    echo -e "${WHITE}Syncing: resources/install_tree → build${NC}"
else
    SRC_CONFIG="$BUILD_CONFIG"
    DST_CONFIG="$TREE_CONFIG"
    SRC_BIN="$BUILD_BIN"
    DST_BIN="$TREE_BIN"
    echo -e "${WHITE}Syncing: build → resources/install_tree${NC}"
fi
echo ""

# ── Sync TOML files ──────────────────────────────────────────────────────────
synced=0
created=0
unchanged=0
diffcount=0

while IFS= read -r -d '' src_file; do
    rel="${src_file#"$SRC_CONFIG"/}"
    dst_file="$DST_CONFIG/$rel"

    if [ "$MODE" = "diff" ]; then
        if [ ! -f "$dst_file" ]; then
            log_new "$rel (only in source)"
            diffcount=$((diffcount + 1))
        elif ! cmp -s "$src_file" "$dst_file"; then
            log_diff "$rel"
            diff --color=auto -u "$dst_file" "$src_file" | head -20
            echo ""
            diffcount=$((diffcount + 1))
        fi
        continue
    fi

    dst_dir="$(dirname "$dst_file")"
    [ -d "$dst_dir" ] || mkdir -p "$dst_dir"

    if [ ! -f "$dst_file" ]; then
        cp "$src_file" "$dst_file"
        log_new "$rel"
        created=$((created + 1))
    elif ! cmp -s "$src_file" "$dst_file"; then
        cp "$src_file" "$dst_file"
        log_ok "$rel"
        synced=$((synced + 1))
    else
        unchanged=$((unchanged + 1))
    fi
done < <(find "$SRC_CONFIG" -type f -name '*.toml' -print0 | sort -z)

# ── Sync install.sh ──────────────────────────────────────────────────────────
if [ -f "$SRC_BIN/install.sh" ]; then
    dst_install="$DST_BIN/install.sh"
    [ -d "$DST_BIN" ] || mkdir -p "$DST_BIN"

    if [ "$MODE" = "diff" ]; then
        if [ ! -f "$dst_install" ]; then
            log_new "bin/install.sh (only in source)"
            diffcount=$((diffcount + 1))
        elif ! cmp -s "$SRC_BIN/install.sh" "$dst_install"; then
            log_diff "bin/install.sh"
            diffcount=$((diffcount + 1))
        fi
    else
        if [ ! -f "$dst_install" ]; then
            cp "$SRC_BIN/install.sh" "$dst_install"
            log_new "bin/install.sh"
            created=$((created + 1))
        elif ! cmp -s "$SRC_BIN/install.sh" "$dst_install"; then
            cp "$SRC_BIN/install.sh" "$dst_install"
            log_ok "bin/install.sh"
            synced=$((synced + 1))
        else
            unchanged=$((unchanged + 1))
        fi
    fi
fi

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
if [ "$MODE" = "diff" ]; then
    if [ "$diffcount" -eq 0 ]; then
        echo -e "${GREEN}All files in sync.${NC}"
    else
        echo -e "${YELLOW}$diffcount file(s) differ.${NC}"
    fi
else
    echo -e "${WHITE}Done:${NC} $synced updated, $created new, $unchanged unchanged"
fi
