#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# build-linux.sh - Build Maximus on Linux (native or OrbStack)
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# Usage: ./scripts/build-linux.sh [release]
#   release: Also create release package after building
#

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check we're on Linux
if [ "$(uname -s)" != "Linux" ]; then
    log_error "This script must be run on Linux"
    exit 1
fi

ARCH=$(uname -m)
case "$ARCH" in
    aarch64) ARCH="arm64" ;;
    x86_64) ARCH="x86_64" ;;
    *) log_warn "Unknown architecture: $ARCH" ;;
esac

log_info "Building Maximus on Linux ($ARCH)"

# Check for required packages
log_info "Checking dependencies..."
MISSING=""
command -v gcc >/dev/null 2>&1 || MISSING="$MISSING gcc"
command -v make >/dev/null 2>&1 || MISSING="$MISSING make"
command -v yacc >/dev/null 2>&1 || command -v bison >/dev/null 2>&1 || MISSING="$MISSING bison"

# Check for ncurses headers
if [ ! -f /usr/include/ncurses.h ] && [ ! -f /usr/include/curses.h ]; then
    if [ ! -f /usr/include/ncurses/ncurses.h ]; then
        MISSING="$MISSING libncurses-dev"
    fi
fi

if [ -n "$MISSING" ]; then
    log_error "Missing packages:$MISSING"
    log_info "On Debian/Ubuntu: sudo apt-get install build-essential bison libncurses-dev"
    log_info "On Alpine: apk add build-base bison ncurses-dev"
    log_info "On RHEL/Fedora: sudo dnf install gcc make bison ncurses-devel"
    exit 1
fi

# Run configure to generate vars.mk for this platform
# Use local build/ directory, not /var/max
log_info "Running configure with PREFIX=$PROJECT_ROOT/build..."
./configure --prefix="$PROJECT_ROOT/build"

# Clean and build
log_info "Cleaning previous build..."
make clean 2>/dev/null || true

# Remove any cross-platform binaries (e.g., macOS Mach-O files on Linux)
log_info "Removing old binaries from build directory..."
rm -rf "$PROJECT_ROOT/build/lib/"*.so "$PROJECT_ROOT/build/lib/"*.dylib 2>/dev/null || true
rm -rf "$PROJECT_ROOT/build/bin/"* 2>/dev/null || true

log_info "Building..."
make build

log_info "Installing to build directory..."
make install

# Build maxtel
log_info "Building maxtel..."
make maxtel_install

log_info "Build complete!"
echo ""
echo "Binaries are in: $PROJECT_ROOT/build/bin/"
echo "Libraries are in: $PROJECT_ROOT/build/lib/"
echo ""

# Create release if requested
if [ "$1" = "release" ]; then
    log_info "Creating release package..."
    ./scripts/make-release.sh "$ARCH"
fi
