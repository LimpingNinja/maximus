#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# make-release.sh - Create a release package for Maximus BBS
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# Usage: ./scripts/make-release.sh [arch]
#   arch: arm64, x86_64, or auto (default: auto-detect)
#

set -e

# Configuration
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Extract version from src/max/core/max_vr.h
VER_MAJ=$(grep '#define VER_MAJ' "$PROJECT_ROOT/src/max/core/max_vr.h" | sed 's/.*VER_MAJ[[:space:]]*"\([^"]*\)".*/\1/')
VER_MIN=$(grep '#define VER_MIN' "$PROJECT_ROOT/src/max/core/max_vr.h" | sed 's/.*VER_MIN[[:space:]]*"\([^"]*\)".*/\1/')
VER_SUFFIX=$(grep '#define VER_SUFFIX' "$PROJECT_ROOT/src/max/core/max_vr.h" | sed 's/.*VER_SUFFIX[[:space:]]*"\([^"]*\)".*/\1/')
VERSION="${VER_MAJ}.${VER_MIN}${VER_SUFFIX}"

if [ -z "$VERSION" ] || [ "$VERSION" = "." ]; then
    echo "Error: Could not extract version from src/max/core/max_vr.h"
    exit 1
fi
BUILD_DIR="${PROJECT_ROOT}/build"
RELEASE_DIR="${PROJECT_ROOT}/release"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Detect architecture
detect_arch() {
    local arch="$1"
    if [ "$arch" = "auto" ] || [ -z "$arch" ]; then
        arch=$(uname -m)
        case "$arch" in
            arm64|aarch64) arch="arm64" ;;
            x86_64|amd64) arch="x86_64" ;;
            i386|i686) arch="x86" ;;
            *) log_error "Unknown architecture: $arch"; exit 1 ;;
        esac
    fi
    echo "$arch"
}

# Detect OS
detect_os() {
    local os=$(uname -s)
    case "$os" in
        Darwin) echo "macos" ;;
        Linux) echo "linux" ;;
        *) echo "unknown" ;;
    esac
}

# Build for specific architecture
build_for_arch() {
    local arch="$1"
    local cross_compile="$2"
    
    log_info "Building for architecture: $arch"
    
    cd "$PROJECT_ROOT"
    
    # For cross-compilation, set architecture-specific flags
    if [ "$cross_compile" = "true" ]; then
        log_warn "Cross-compilation requested - this may require additional toolchains"
        case "$arch" in
            x86_64)
                if [ "$(detect_os)" = "macos" ]; then
                    export CFLAGS="-arch x86_64"
                    export CXXFLAGS="-arch x86_64"
                    export LDFLAGS="-arch x86_64"
                    # Force use of x86_64 compiler
                    export CC="clang -arch x86_64"
                    export CXX="clang++ -arch x86_64"
                fi
                ;;
            x86)
                export CFLAGS="-m32"
                export CXXFLAGS="-m32"
                export LDFLAGS="-m32"
                ;;
        esac
    else
        # Clear any previous cross-compile settings
        unset CFLAGS CXXFLAGS LDFLAGS CC CXX
    fi
    
    # For cross-compilation, use archclean to remove old libs
    if [ "$cross_compile" = "true" ]; then
        log_info "Cross-architecture build: $arch"
        log_info "Running archclean to remove old architecture libraries..."
        make archclean
    else
        log_info "Cleaning previous build..."
        make clean
    fi
    
    # Rebuild everything with ARCH variable
    # Use 'build' + 'install' instead of 'all' because 'all' runs clean again
    log_info "Building all components for $arch..."
    make ARCH="$arch" build
    make ARCH="$arch" install
    
    # Build maxtel supervisor
    log_info "Building maxtel..."
    make ARCH="$arch" maxtel_install
    
    # Codesign on macOS
    if [ "$(detect_os)" = "macos" ]; then
        log_info "Codesigning binaries..."
        for bin in "${BUILD_DIR}/bin/"*; do
            if [ -f "$bin" ] && file "$bin" | grep -q "Mach-O"; then
                codesign --force --sign - "$bin" 2>/dev/null || true
            fi
        done
        for lib in "${BUILD_DIR}/bin/lib/"*.so "${BUILD_DIR}/bin/lib/"*.dylib; do
            if [ -f "$lib" ]; then
                codesign --force --sign - "$lib" 2>/dev/null || true
            fi
        done
    fi
}

# Create release package
create_release_package() {
    local arch="$1"
    local os="$2"
    local release_name="maximus-${VERSION}-${os}-${arch}"
    local release_path="${RELEASE_DIR}/${release_name}"
    
    log_info "Creating release package: $release_name"
    
    # Remove existing release dir if present
    rm -rf "$release_path"
    mkdir -p "$release_path"
    
    # Copy fresh install_tree as base (clean config, no user data)
    log_info "Copying fresh install_tree..."
    cp -rp "${PROJECT_ROOT}/resources/install_tree/"* "$release_path/"
    
    # Replace /var/max paths with relative paths for portable release
    log_info "Updating config paths..."
    for file in config/legacy/max.ctl config/legacy/areas.bbs config/compress.cfg config/squish.cfg config/sqafix.cfg; do
        if [ -f "$release_path/$file" ]; then
            # Replace both /var/max/ and /var/max (with or without trailing slash)
            LC_ALL=C sed -i.bak -e "s;/var/max/;./;g" -e "s;/var/max$;.;g" -e "s;/var/max\([^/]\);./\1;g" "$release_path/$file"
            rm -f "$release_path/$file.bak"
        fi
    done
    
    # Copy binaries (overwrites empty bin/ from install_tree)
    log_info "Copying binaries..."
    cp -f "${BUILD_DIR}/bin/"* "$release_path/bin/" 2>/dev/null || true

    # Copy SQLite userdb init resources (schema + wrapper)
    mkdir -p "$release_path/data/db"
    mkdir -p "$release_path/data/users"
    cp -f "${PROJECT_ROOT}/scripts/db/userdb_schema.sql" "$release_path/data/db/userdb_schema.sql" 2>/dev/null || true
    cp -f "${PROJECT_ROOT}/scripts/db/init-userdb.sh" "$release_path/bin/init-userdb.sh" 2>/dev/null || true
    chmod +x "$release_path/bin/init-userdb.sh" 2>/dev/null || true
    
    # Copy libraries (into bin/lib/)
    log_info "Copying libraries..."
    mkdir -p "$release_path/bin/lib"
    cp -f "${BUILD_DIR}/bin/lib/"*.so "$release_path/bin/lib/" 2>/dev/null || true
    cp -f "${BUILD_DIR}/bin/lib/"*.dylib "$release_path/bin/lib/" 2>/dev/null || true
    
    # Codesign and clear quarantine on macOS
    if [ "$os" = "macos" ]; then
        log_info "Codesigning release binaries..."
        xattr -cr "$release_path/bin" "$release_path/bin/lib" 2>/dev/null || true
        for bin in "$release_path/bin/"*; do
            if [ -f "$bin" ] && file "$bin" | grep -q "Mach-O"; then
                codesign --force --sign - "$bin" 2>/dev/null || true
            fi
        done
        for lib in "$release_path/bin/lib/"*.so "$release_path/bin/lib/"*.dylib; do
            if [ -f "$lib" ]; then
                codesign --force --sign - "$lib" 2>/dev/null || true
            fi
        done
    fi
    
    # Copy compiled display files (.bbs) from build
    log_info "Copying compiled display files..."
    cp -f "${BUILD_DIR}/display/screens/"*.bbs "$release_path/display/screens/" 2>/dev/null || true
    cp -f "${BUILD_DIR}/display/help/"*.bbs "$release_path/display/help/" 2>/dev/null || true
    
    # Copy language TOML files
    log_info "Copying language files..."
    cp -f "${BUILD_DIR}/config/lang/"*.toml "$release_path/config/lang/" 2>/dev/null || true
    
    # Copy compiled MEX files (.vm)
    log_info "Copying compiled MEX files..."
    cp -f "${BUILD_DIR}/scripts/"*.vm "$release_path/scripts/" 2>/dev/null || true
    
    # Copy documentation
    log_info "Copying documentation..."
    mkdir -p "$release_path/docs"
    cp -f "${PROJECT_ROOT}/docs/"*.md "$release_path/docs/" 2>/dev/null || true
    cp -f "${PROJECT_ROOT}/docs/"*.txt "$release_path/docs/" 2>/dev/null || true
    cp -f "${PROJECT_ROOT}/README"* "$release_path/docs/" 2>/dev/null || true
    cp -f "${PROJECT_ROOT}/LICENSE"* "$release_path/docs/" 2>/dev/null || true
    cp -f "${PROJECT_ROOT}/COPYING"* "$release_path/docs/" 2>/dev/null || true
    
    # Create run script at root level
    cat > "$release_path/runbbs.sh" << 'EOF'
#!/bin/bash
# Maximus BBS launcher — starts maxtel supervisor
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="${SCRIPT_DIR}/bin/lib:${LD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH="${SCRIPT_DIR}/bin/lib:${DYLD_LIBRARY_PATH}"
export MEX_INCLUDE="${SCRIPT_DIR}/scripts/include"
export MAX_INSTALL_PATH="${SCRIPT_DIR}"
cd "$SCRIPT_DIR"
exec bin/maxtel -d "$SCRIPT_DIR" -p 2323 -n 4 "$@"
EOF
    chmod +x "$release_path/runbbs.sh"
    
    # Create recompile script for end users
    cat > "$release_path/bin/recompile.sh" << 'EOF'
#!/bin/bash
# Recompile all Maximus display and MEX files
# Run this after modifying .mec or .mex files

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"
cd "$BASE_DIR"

export LD_LIBRARY_PATH="${BASE_DIR}/bin/lib:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="${BASE_DIR}/bin/lib:$DYLD_LIBRARY_PATH"
export MEX_INCLUDE="${BASE_DIR}/scripts/include"

echo "=== Recompiling Maximus Configuration ==="
echo

echo "Step 1: Compiling help display files (.mec -> .bbs)..."
(cd display/help && for f in *.mec; do ../../bin/mecca "$f" 2>&1 || true; done)

echo "Step 2: Compiling screen display files (.mec -> .bbs)..."
(cd display/screens && for f in *.mec; do ../../bin/mecca "$f" 2>&1 || true; done)

echo "Step 3: Compiling MEX scripts (.mex -> .vm)..."
(cd scripts && for f in *.mex; do ../bin/mex "$f" 2>&1 || true; done)

echo
echo "=== Recompilation complete ==="
EOF
    chmod +x "$release_path/bin/recompile.sh"
    
    # Copy telnet helper scripts
    log_info "Copying telnet scripts..."
    cp -f "${PROJECT_ROOT}/scripts/max-node.sh" "$release_path/bin/"
    cp -f "${PROJECT_ROOT}/scripts/start-nodes.sh" "$release_path/bin/"
    chmod +x "$release_path/bin/max-node.sh" "$release_path/bin/start-nodes.sh"
    
    # Create README for release
    cat > "$release_path/README.txt" << EOF
Maximus BBS v${VERSION}
======================

Platform: ${os} (${arch})
Build Date: $(date +%Y-%m-%d)

Quick Start (Local Mode):
------------------------
  bin/runbbs.sh -c

Quick Start (Telnet via MAXTEL):
-------------------------------
  bin/maxtel -p 2323 -n 4

  Then connect: telnet localhost 2323

  For headless/daemon operation:
    bin/maxtel -H -p 2323 -n 4     # Headless (no UI)
    bin/maxtel -D -p 2323 -n 4     # Daemon (background)

The release comes with pre-compiled configuration files. If you need to 
modify any configuration, edit the source files and run bin/recompile.sh

Directory Structure:
--------------------
  runbbs.sh      - Primary BBS launcher (starts maxtel)
  bin/           - Executables (max, maxtel, maxcfg, mex, mecca, squish, etc.)
    lib/         - Shared libraries
  config/        - Configuration (TOML is sole authority)
    maximus.toml - Core system config
    general/     - General BBS settings (session, equipment, etc.)
    lang/        - Language files (TOML)
    menus/       - Menu definitions (TOML)
    security/    - Access control
    legacy/      - Legacy .ctl files (read-only reference)
  display/       - User-facing display content
    screens/     - System screens (.mec source, .bbs compiled)
    help/        - Help screens
    menus/       - Menu display art
    tunes/       - Tune files
  scripts/       - MEX scripts (.mex source, .vm compiled)
    include/     - MEX headers (.mh, .lh)
  data/          - Persistent data (users, msgbase, mail, etc.)
  run/           - Ephemeral runtime state (node dirs, tmp)
  log/           - Log files
  doors/         - External door programs
  docs/          - Documentation

After Modifying Display or MEX Files:
-------------------------------------
If you edit .mec or .mex files, run:
  bin/recompile.sh

Or manually:
  bin/mecca display/screens/*.mec   # Recompile display files
  bin/mex scripts/script.mex        # Recompile a MEX script

MAXTEL (Telnet Supervisor):
--------------------------
  bin/maxtel             - Multi-node telnet supervisor with ncurses UI
    -p PORT              - Listen port (default: 2323)
    -n NODES             - Number of nodes (default: 4)
    -H                   - Headless mode (no UI, for scripts)
    -D                   - Daemon mode (fork to background)
    -h                   - Show all options

  See docs/maxtel.md for full documentation.

Legacy Telnet Scripts:
---------------------
  bin/max-node.sh <N>    - Run a single node in a loop (use inside screen/tmux)
  bin/start-nodes.sh [n] - Start n nodes using screen (default: 4)
  bin/maxcomm            - Bridges telnet connections to Maximus sockets

Environment Variables:
---------------------
  MEX_INCLUDE      - Path to MEX include files (set by runbbs.sh)
  MAX_INSTALL_PATH - Override install path (default: current directory)
  AFTER_MAX        - Optional command to run after max exits

For more information, see docs/BUILD.md

EOF
    
    # Run recompile.sh to ensure all config files are freshly compiled
    log_info "Running recompile.sh in release directory..."
    (cd "$release_path" && ./bin/recompile.sh)
    
    # Create tarball
    log_info "Creating tarball..."
    cd "$RELEASE_DIR"
    tar -czf "${release_name}.tar.gz" "$release_name"
    
    log_info "Release package created: ${RELEASE_DIR}/${release_name}.tar.gz"
    
    # Print summary
    echo ""
    echo "=========================================="
    echo "Release Summary"
    echo "=========================================="
    echo "Name:     $release_name"
    echo "Path:     $release_path"
    echo "Tarball:  ${release_name}.tar.gz"
    echo "Size:     $(du -sh "${release_name}.tar.gz" | cut -f1)"
    echo ""
}

# Main
main() {
    local requested_arch="${1:-auto}"
    local arch=$(detect_arch "$requested_arch")
    local os=$(detect_os)
    local cross_compile="false"
    
    echo "=========================================="
    echo "Maximus BBS Release Builder"
    echo "=========================================="
    echo "Version:      $VERSION"
    echo "Architecture: $arch"
    echo "OS:           $os"
    echo "=========================================="
    echo ""
    
    # Check if cross-compilation is needed
    local native_arch=$(uname -m)
    case "$native_arch" in
        arm64|aarch64) native_arch="arm64" ;;
        x86_64|amd64) native_arch="x86_64" ;;
    esac
    
    if [ "$arch" != "$native_arch" ]; then
        cross_compile="true"
        log_warn "Cross-compilation mode enabled ($native_arch -> $arch)"
    fi
    
    # Build
    build_for_arch "$arch" "$cross_compile"
    
    # Package
    create_release_package "$arch" "$os"
    
    log_info "Done!"
}

# Run main with arguments
main "$@"
