#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# mex-compile.sh - Compile a single MEX script by name
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# Usage (from BBS root directory):
#   ./bin/mex-compile.sh learn/learn-file-io
#   ./bin/mex-compile.sh oneliner
#   ./bin/mex-compile.sh learn/learn-user-record
#
# The argument is relative to scripts/, without the .mex extension.
# The compiled .vm is placed alongside the .mex source.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"

if [ -z "$1" ]; then
    echo "Usage: $0 <script-name>"
    echo ""
    echo "  script-name  Path relative to scripts/, without .mex extension"
    echo ""
    echo "Examples:"
    echo "  $0 oneliner"
    echo "  $0 learn/learn-file-io"
    echo "  $0 learn/learn-user-record"
    exit 1
fi

SCRIPT_NAME="$1"
MEX_FILE="${BASE_DIR}/scripts/${SCRIPT_NAME}.mex"

if [ ! -f "$MEX_FILE" ]; then
    echo "Error: ${MEX_FILE} not found"
    exit 1
fi

export LD_LIBRARY_PATH="${BASE_DIR}/lib:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="${BASE_DIR}/lib:$DYLD_LIBRARY_PATH"
export MEX_INCLUDE="${BASE_DIR}/scripts/include"

MEX_DIR="$(dirname "$MEX_FILE")"
MEX_BASE="$(basename "$MEX_FILE")"

echo "Compiling ${SCRIPT_NAME}.mex ..."
(cd "$MEX_DIR" && "${BASE_DIR}/bin/mex" "$MEX_BASE")
echo "Done: scripts/${SCRIPT_NAME}.vm"
