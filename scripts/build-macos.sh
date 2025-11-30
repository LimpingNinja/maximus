#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# build-macos.sh - Build Maximus on macOS (arm64 or x86_64)
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# Usage: ./scripts/build-macos.sh [arch] [release]
#   arch: arm64, x86_64, or auto (default: auto-detect)
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

# Check we're on macOS
if [ "$(uname -s)" != "Darwin" ]; then
    log_error "This script must be run on macOS"
    exit 1
fi

# Parse arguments
REQUESTED_ARCH=""
DO_RELEASE=false

for arg in "$@"; do
    case "$arg" in
        arm64|x86_64|auto) REQUESTED_ARCH="$arg" ;;
        release) DO_RELEASE=true ;;
        -h|--help)
            echo "Usage: $0 [arch] [release]"
            echo "  arch: arm64, x86_64, or auto (default: auto-detect)"
            echo "  release: Also create release package after building"
            echo ""
            echo "Examples:"
            echo "  $0              # Build for native architecture"
            echo "  $0 arm64        # Build for Apple Silicon"
            echo "  $0 x86_64       # Build for Intel (via Rosetta on AS)"
            echo "  $0 release      # Build and create release package"
            echo "  $0 x86_64 release"
            exit 0
            ;;
        *) log_warn "Unknown argument: $arg" ;;
    esac
done

# Detect native architecture
NATIVE_ARCH=$(uname -m)
case "$NATIVE_ARCH" in
    arm64) NATIVE_ARCH="arm64" ;;
    x86_64) NATIVE_ARCH="x86_64" ;;
    *) log_error "Unknown native architecture: $NATIVE_ARCH"; exit 1 ;;
esac

# Determine target architecture
if [ -z "$REQUESTED_ARCH" ] || [ "$REQUESTED_ARCH" = "auto" ]; then
    ARCH="$NATIVE_ARCH"
else
    ARCH="$REQUESTED_ARCH"
fi

# Check if cross-compilation is needed
CROSS_COMPILE=false
if [ "$ARCH" != "$NATIVE_ARCH" ]; then
    CROSS_COMPILE=true
    log_warn "Cross-compilation: $NATIVE_ARCH -> $ARCH"
    
    if [ "$ARCH" = "x86_64" ] && [ "$NATIVE_ARCH" = "arm64" ]; then
        # Check Rosetta
        if ! /usr/bin/arch -x86_64 /usr/bin/true 2>/dev/null; then
            log_error "Rosetta 2 is required for x86_64 builds on Apple Silicon"
            log_info "Install with: softwareupdate --install-rosetta"
            exit 1
        fi
    elif [ "$ARCH" = "arm64" ] && [ "$NATIVE_ARCH" = "x86_64" ]; then
        log_error "Cannot build arm64 on Intel Mac"
        exit 1
    fi
fi

log_info "Building Maximus on macOS ($ARCH)"

# Check for required tools
log_info "Checking dependencies..."
MISSING=""
command -v clang >/dev/null 2>&1 || command -v gcc >/dev/null 2>&1 || MISSING="$MISSING clang/gcc"
command -v make >/dev/null 2>&1 || MISSING="$MISSING make"
command -v yacc >/dev/null 2>&1 || command -v bison >/dev/null 2>&1 || MISSING="$MISSING bison"

if [ -n "$MISSING" ]; then
    log_error "Missing tools:$MISSING"
    log_info "Install Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

# Set up cross-compilation environment
if [ "$CROSS_COMPILE" = true ] && [ "$ARCH" = "x86_64" ]; then
    log_info "Setting up x86_64 cross-compilation via Rosetta..."
    export CC="clang -arch x86_64"
    export CXX="clang++ -arch x86_64"
    export CFLAGS="-arch x86_64"
    export CXXFLAGS="-arch x86_64"
    export LDFLAGS="-arch x86_64"
fi

# Run configure to generate vars.mk for this platform
log_info "Running configure with PREFIX=$PROJECT_ROOT/build..."
if [ "$CROSS_COMPILE" = true ] && [ "$ARCH" = "x86_64" ]; then
    /usr/bin/arch -x86_64 /bin/bash -c "./configure --prefix='$PROJECT_ROOT/build'"
else
    ./configure --prefix="$PROJECT_ROOT/build"
fi

# Clean previous build
log_info "Cleaning previous build..."
make clean 2>/dev/null || true

# Remove old binaries that might be wrong architecture
log_info "Removing old binaries from build directory..."
rm -rf "$PROJECT_ROOT/build/lib/"*.so "$PROJECT_ROOT/build/lib/"*.dylib 2>/dev/null || true
rm -rf "$PROJECT_ROOT/build/bin/"* 2>/dev/null || true

# Build
log_info "Building for $ARCH..."
if [ "$CROSS_COMPILE" = true ] && [ "$ARCH" = "x86_64" ]; then
    /usr/bin/arch -x86_64 /bin/bash -c "make ARCH=$ARCH build"
    /usr/bin/arch -x86_64 /bin/bash -c "make ARCH=$ARCH install"
    /usr/bin/arch -x86_64 /bin/bash -c "make ARCH=$ARCH maxtel_install"
else
    make ARCH="$ARCH" build
    make ARCH="$ARCH" install
    make ARCH="$ARCH" maxtel_install
fi

# Codesign binaries (ad-hoc signing for local use)
log_info "Codesigning binaries..."
for bin in "$PROJECT_ROOT/build/bin/"*; do
    if [ -f "$bin" ] && file "$bin" | grep -q "Mach-O"; then
        codesign --force --sign - "$bin" 2>/dev/null || true
    fi
done
for lib in "$PROJECT_ROOT/build/lib/"*.dylib "$PROJECT_ROOT/build/lib/"*.so; do
    if [ -f "$lib" ]; then
        codesign --force --sign - "$lib" 2>/dev/null || true
    fi
done

log_info "Build complete!"
echo ""
echo "Architecture: $ARCH"
echo "Binaries are in: $PROJECT_ROOT/build/bin/"
echo "Libraries are in: $PROJECT_ROOT/build/lib/"
echo ""

# Verify architecture
SAMPLE_BIN=$(ls "$PROJECT_ROOT/build/bin/"* 2>/dev/null | head -1)
if [ -n "$SAMPLE_BIN" ]; then
    BUILT_ARCH=$(file "$SAMPLE_BIN" | grep -o 'arm64\|x86_64' | head -1)
    if [ "$BUILT_ARCH" = "$ARCH" ]; then
        log_info "Verified: binaries are $BUILT_ARCH"
    else
        log_warn "Expected $ARCH but got $BUILT_ARCH"
    fi
fi

# Create release if requested
if [ "$DO_RELEASE" = true ]; then
    log_info "Creating release package..."
    ./scripts/make-release.sh "$ARCH"
fi
