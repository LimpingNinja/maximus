#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# recompile.sh - Recompile all Maximus configuration files
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# Run this after modifying .mec, .mad, or .mex files

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"
cd "$BASE_DIR"

export LD_LIBRARY_PATH="${BASE_DIR}/lib:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="${BASE_DIR}/lib:$DYLD_LIBRARY_PATH"
export MEX_INCLUDE="${BASE_DIR}/m"

echo "=== Recompiling Maximus Configuration ==="
echo

echo "Step 1: Compiling language file (english.mad)..."
(cd etc/lang && ../../bin/maid english)

echo "Step 2: Compiling help display files (.mec -> .bbs)..."
for f in etc/help/*.mec; do
    [ -f "$f" ] && bin/mecca "$f"
done

echo "Step 3: Compiling misc display files (.mec -> .bbs)..."
for f in etc/misc/*.mec; do 
    [ -f "$f" ] && bin/mecca "$f"
done

echo "Step 4: Compiling MEX scripts (.mex -> .vm)..."
(cd m && for f in *.mex; do ../bin/mex "$f" 2>&1 || true; done)

echo "Step 5: Re-linking language file..."
(cd etc/lang && ../../bin/maid english -d -s -p../max)

echo
echo "=== Recompilation complete ==="
