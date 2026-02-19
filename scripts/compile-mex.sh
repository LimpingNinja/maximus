#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# compile-mex.sh - Compile one MEX script and copy outputs
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# Usage:
#   ./scripts/compile-mex.sh <script-name|script-name.mex> [--deploy]
#
# Behavior:
#   - Compiles the target .mex in resources/m using build/bin/mex
#   - Copies the compiled .vm to build/scripts
#   - With --deploy: also copies .mex and .vm to resources/install_tree/scripts
#

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MEX_SRC_DIR="$PROJECT_ROOT/resources/m"
BUILD_SCRIPTS_DIR="$PROJECT_ROOT/build/scripts"
INSTALL_SCRIPTS_DIR="$PROJECT_ROOT/resources/install_tree/scripts"
MEX_COMPILER="$PROJECT_ROOT/build/bin/mex"

usage() {
  echo "Usage: $0 <script-name|script-name.mex> [--deploy]"
  echo "Example: $0 footerfile --deploy"
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

SCRIPT_ARG=""
DEPLOY=0

for arg in "$@"; do
  case "$arg" in
    --deploy)
      DEPLOY=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    -* )
      echo "Unknown option: $arg"
      usage
      exit 1
      ;;
    *)
      if [ -n "$SCRIPT_ARG" ]; then
        echo "Error: only one script name may be provided"
        usage
        exit 1
      fi
      SCRIPT_ARG="$arg"
      ;;
  esac
done

if [ -z "$SCRIPT_ARG" ]; then
  echo "Error: missing script name"
  usage
  exit 1
fi

SCRIPT_FILE="$SCRIPT_ARG"
case "$SCRIPT_FILE" in
  *.mex) ;;
  *) SCRIPT_FILE="${SCRIPT_FILE}.mex" ;;
esac

SCRIPT_BASENAME="${SCRIPT_FILE%.mex}"
MEX_SOURCE_PATH="$MEX_SRC_DIR/$SCRIPT_FILE"
VM_OUTPUT_PATH="$MEX_SRC_DIR/${SCRIPT_BASENAME}.vm"

if [ ! -f "$MEX_SOURCE_PATH" ]; then
  echo "Error: MEX source not found: $MEX_SOURCE_PATH"
  exit 1
fi

if [ ! -x "$MEX_COMPILER" ]; then
  echo "Error: MEX compiler not found or not executable: $MEX_COMPILER"
  exit 1
fi

mkdir -p "$BUILD_SCRIPTS_DIR"
mkdir -p "$INSTALL_SCRIPTS_DIR"

if [ "$DEPLOY" -eq 1 ]; then
  cp -f "$MEX_SOURCE_PATH" "$BUILD_SCRIPTS_DIR/$SCRIPT_FILE"
  cp -f "$MEX_SOURCE_PATH" "$INSTALL_SCRIPTS_DIR/$SCRIPT_FILE"
fi

(
  cd "$MEX_SRC_DIR"
  ../../build/bin/mex "$SCRIPT_FILE"
)

if [ ! -f "$VM_OUTPUT_PATH" ]; then
  echo "Error: compile completed but VM output not found: $VM_OUTPUT_PATH"
  exit 1
fi

cp -f "$VM_OUTPUT_PATH" "$BUILD_SCRIPTS_DIR/${SCRIPT_BASENAME}.vm"

if [ "$DEPLOY" -eq 1 ]; then
  cp -f "$VM_OUTPUT_PATH" "$INSTALL_SCRIPTS_DIR/${SCRIPT_BASENAME}.vm"
fi

echo "Done: compiled $SCRIPT_FILE"
echo " - VM copied to: $BUILD_SCRIPTS_DIR/${SCRIPT_BASENAME}.vm"
if [ "$DEPLOY" -eq 1 ]; then
  echo " - Deployed MEX to: $INSTALL_SCRIPTS_DIR/$SCRIPT_FILE"
  echo " - Deployed VM to:  $INSTALL_SCRIPTS_DIR/${SCRIPT_BASENAME}.vm"
fi
