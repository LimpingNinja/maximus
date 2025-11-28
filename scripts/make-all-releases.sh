#!/bin/bash
#
# make-all-releases.sh - Build releases for multiple architectures
#
# On macOS (Apple Silicon), can build:
#   - arm64 (native)
#   - x86_64 (via Rosetta)
#
# With mingw-w64 installed, can cross-compile for Windows
#

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT_DIR="${PROJECT_ROOT}/scripts"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Parse arguments
BUILD_MACOS=true
BUILD_WINDOWS=false
BUILD_X86_64=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --windows) BUILD_WINDOWS=true; shift ;;
        --x86_64) BUILD_X86_64=true; shift ;;
        --all) BUILD_WINDOWS=true; BUILD_X86_64=true; shift ;;
        --help)
            echo "Usage: $0 [options]"
            echo "  --windows   Also attempt Windows cross-compile (requires mingw-w64)"
            echo "  --x86_64    Also build macOS x86_64 (requires Rosetta)"
            echo "  --all       Build all available platforms"
            exit 0
            ;;
        *) shift ;;
    esac
done

echo "=========================================="
echo "Maximus BBS Multi-Architecture Release"
echo "=========================================="
echo ""

OS=$(uname -s)
NATIVE_ARCH=$(uname -m)

# Build native macOS
if [ "$BUILD_MACOS" = true ]; then
    case "$OS" in
        Darwin)
            log_info "Building on macOS"
            
            if [ "$NATIVE_ARCH" = "arm64" ]; then
                log_info "Building native arm64..."
                "$SCRIPT_DIR/make-release.sh" arm64
                
                # Try x86_64 via Rosetta
                if [ "$BUILD_X86_64" = true ]; then
                    if [ -f /usr/bin/arch ] && /usr/bin/arch -x86_64 /usr/bin/true 2>/dev/null; then
                        log_info "Building x86_64 via Rosetta..."
                        /usr/bin/arch -x86_64 /bin/bash -c "cd '$PROJECT_ROOT' && '$SCRIPT_DIR/make-release.sh' x86_64" || {
                            log_warn "x86_64 build failed"
                        }
                    else
                        log_warn "Rosetta not available for x86_64 build"
                    fi
                fi
            else
                # On Intel Mac
                log_info "Building native x86_64..."
                "$SCRIPT_DIR/make-release.sh" x86_64
            fi
            ;;
            
        Linux)
            log_info "Building on Linux"
            "$SCRIPT_DIR/make-release.sh" auto
            ;;
            
        *)
            log_warn "Unknown OS: $OS"
            "$SCRIPT_DIR/make-release.sh" auto
            ;;
    esac
fi

# Windows cross-compile
if [ "$BUILD_WINDOWS" = true ]; then
    echo ""
    log_info "Attempting Windows cross-compilation..."
    
    if command -v x86_64-w64-mingw32-gcc &>/dev/null; then
        log_info "mingw-w64 found, testing cross-compile..."
        
        # Test compile
        cd "$PROJECT_ROOT"
        make -f Makefile.mingw ARCH=x86_64 build 2>&1 || {
            log_warn "Windows cross-compilation not fully supported yet"
            log_warn "The codebase needs Unix->Windows porting work"
        }
    else
        log_warn "mingw-w64 not found. Install with: brew install mingw-w64"
    fi
fi

echo ""
echo "=========================================="
echo "Build Complete"
echo "=========================================="
echo ""
log_info "Release packages:"
ls -la "${PROJECT_ROOT}/release/"*.tar.gz 2>/dev/null || echo "No packages found"
echo ""
