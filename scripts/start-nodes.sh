#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# start-nodes.sh - Start Maximus nodes using screen
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja

NUM_NODES=${1:-4}
MAX_DIR="${MAX_INSTALL_PATH:-$(cd "$(dirname "$0")/.." && pwd)}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

cd "$MAX_DIR" || exit 1

echo "Starting $NUM_NODES Maximus nodes..."

for i in $(seq 1 $NUM_NODES); do
    screen -dmS "max$i" "$SCRIPT_DIR/max-node.sh" "$i"
    echo "  Started node $i (screen session: max$i)"
done

echo ""
echo "Nodes started. Use 'screen -ls' to see sessions."
echo "Attach with: screen -r max<N>"
echo ""
echo "To start telnet listener:"
echo "  socat TCP-LISTEN:2323,fork,reuseaddr EXEC:\"$MAX_DIR/bin/maxcomm\""
