#!/bin/bash
#
# make-release.sh - Create a release package for Maximus BBS
#
# Usage: ./scripts/make-release.sh [arch]
#   arch: arm64, x86_64, or auto (default: auto-detect)
#

set -e

# Configuration
VERSION="3.03"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
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
        MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
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
    
    # Codesign on macOS
    if [ "$(detect_os)" = "macos" ]; then
        log_info "Codesigning binaries..."
        for bin in "${BUILD_DIR}/bin/"*; do
            if [ -f "$bin" ] && file "$bin" | grep -q "Mach-O"; then
                codesign --force --sign - "$bin" 2>/dev/null || true
            fi
        done
        for lib in "${BUILD_DIR}/lib/"*.so "${BUILD_DIR}/lib/"*.dylib; do
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
    
    # Create directory structure
    mkdir -p "$release_path/bin"
    mkdir -p "$release_path/lib"
    mkdir -p "$release_path/etc"
    mkdir -p "$release_path/m"
    mkdir -p "$release_path/log"
    mkdir -p "$release_path/docs"
    
    # Copy binaries
    log_info "Copying binaries..."
    cp -f "${BUILD_DIR}/bin/"* "$release_path/bin/" 2>/dev/null || true
    
    # Copy libraries
    log_info "Copying libraries..."
    cp -f "${BUILD_DIR}/lib/"*.so "$release_path/lib/" 2>/dev/null || true
    cp -f "${BUILD_DIR}/lib/"*.dylib "$release_path/lib/" 2>/dev/null || true
    
    # Copy configuration
    log_info "Copying configuration..."
    cp -rf "${BUILD_DIR}/etc/"* "$release_path/etc/" 2>/dev/null || true
    
    # Copy MEX files
    log_info "Copying MEX files..."
    cp -rf "${BUILD_DIR}/m/"* "$release_path/m/" 2>/dev/null || true
    
    # Copy documentation
    log_info "Copying documentation..."
    cp -f "${PROJECT_ROOT}/docs/"*.md "$release_path/docs/" 2>/dev/null || true
    cp -f "${PROJECT_ROOT}/README"* "$release_path/docs/" 2>/dev/null || true
    cp -f "${PROJECT_ROOT}/LICENSE"* "$release_path/docs/" 2>/dev/null || true
    cp -f "${PROJECT_ROOT}/COPYING"* "$release_path/docs/" 2>/dev/null || true
    
    # Create run script
    cat > "$release_path/bin/runbbs.sh" << 'EOF'
#!/bin/bash
# Maximus BBS launcher
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"
export LD_LIBRARY_PATH="${BASE_DIR}/lib:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="${BASE_DIR}/lib:$DYLD_LIBRARY_PATH"
cd "$BASE_DIR"
exec "${SCRIPT_DIR}/max" etc/max "$@"
EOF
    chmod +x "$release_path/bin/runbbs.sh"
    
    # Create README for release
    cat > "$release_path/README.txt" << EOF
Maximus BBS v${VERSION}
======================

Platform: ${os} (${arch})
Build Date: $(date +%Y-%m-%d)

Quick Start:
1. Edit etc/max.ctl to configure your BBS
2. Run: bin/silt etc/max -p
3. Run: bin/runbbs.sh

Directory Structure:
  bin/    - Executables
  lib/    - Shared libraries
  etc/    - Configuration files
  m/      - MEX scripts and includes
  log/    - Log files (created at runtime)
  docs/   - Documentation

For more information, see docs/BUILD.md and docs/64BIT_FIXES.md

EOF
    
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
