#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# max-node.sh - Run a Maximus node in a loop (for use with screen/tmux)
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja

TASK=${1:-1}
MAX_DIR="${MAX_INSTALL_PATH:-$(cd "$(dirname "$0")/.." && pwd)}"

cd "$MAX_DIR" || exit 1

while true; do
    ./bin/max -w -pt$TASK -n$TASK -b38400 etc/max.prm
    ERR=$?
    echo "Max node $TASK exited with code $ERR"
    
    if [ $ERR -eq 1 ]; then
        echo "Clean exit, stopping node $TASK"
        exit 0
    fi
    
    sleep 3
done
