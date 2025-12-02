#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# install.sh - Maximus BBS Installation Script
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# This script configures a fresh Maximus installation by:
# - Setting absolute paths in configuration files
# - Optionally configuring BBS name and sysop name
# - Compiling all configuration files
#
# Upgrade mode (--upgrade <path>):
# - Copies bin/ and lib/ files to existing installation
# - Copies missing m/, etc/ansi/, etc/misc/ files
# - Does NOT overwrite existing etc/ config files
#

set -e

# Mode flags
UPGRADE_MODE=0
UPGRADE_PATH=""

# Colors (matching WFC style)
CYAN='\033[0;36m'
LCYAN='\033[1;36m'
YELLOW='\033[1;33m'
WHITE='\033[1;37m'
GREEN='\033[0;32m'
LGREEN='\033[1;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
LBLUE='\033[1;34m'
MAGENTA='\033[0;35m'
LMAGENTA='\033[1;35m'
NC='\033[0m'

# Get script directory and base directory
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_NAME="$(basename "$0")"

# Check if running from bin directory
if [ "$(basename "$SCRIPT_DIR")" = "bin" ]; then
    BASE_DIR="$(dirname "$SCRIPT_DIR")"
else
    echo -e "${RED}Error:${NC} Don't run directly from bin/, run from one level up:"
    echo -e "  ${CYAN}cd ..${NC}"
    echo -e "  ${CYAN}bin/$SCRIPT_NAME${NC}"
    exit 1
fi

# Verify we're in the right place
if [ ! -f "$BASE_DIR/etc/max.ctl" ]; then
    echo -e "${RED}Error:${NC} Cannot find etc/max.ctl"
    echo -e "Make sure you're running from the Maximus installation directory."
    exit 1
fi

cd "$BASE_DIR"

# Display logo
display_logo() {
    echo ""
    echo -e "${LBLUE}    ███╗   ███╗ █████╗ ██╗  ██╗██╗███╗   ███╗██╗   ██╗███████╗${NC}"
    echo -e "${LCYAN}    ████╗ ████║██╔══██╗╚██╗██╔╝██║████╗ ████║██║   ██║██╔════╝${NC}"
    echo -e "${CYAN}    ██╔████╔██║███████║ ╚███╔╝ ██║██╔████╔██║██║   ██║███████╗${NC}"
    echo -e "${LCYAN}    ██║╚██╔╝██║██╔══██║ ██╔██╗ ██║██║╚██╔╝██║██║   ██║╚════██║${NC}"
    echo -e "${LBLUE}    ██║ ╚═╝ ██║██║  ██║██╔╝ ██╗██║██║ ╚═╝ ██║╚██████╔╝███████║${NC}"
    echo -e "${BLUE}    ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚═╝     ╚═╝ ╚═════╝ ╚══════╝${NC}"
    echo ""
    echo -e "${WHITE}                    Bulletin Board System${NC}"
    echo -e "${CYAN}                      Installation Script${NC}"
    echo ""
}

# Display section header
section() {
    echo ""
    echo -e "${LCYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${WHITE}  $1${NC}"
    echo -e "${LCYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

# Log functions
log_info() { echo -e "${CYAN}[INFO]${NC} $1"; }
log_ok() { echo -e "${LGREEN}[OK]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Prompt with default value
prompt() {
    local var_name="$1"
    local prompt_text="$2"
    local default_value="$3"
    local value
    
    if [ -n "$default_value" ]; then
        echo -ne "${YELLOW}$prompt_text${NC} [${WHITE}$default_value${NC}]: "
    else
        echo -ne "${YELLOW}$prompt_text${NC}: "
    fi
    read value
    
    if [ -z "$value" ] && [ -n "$default_value" ]; then
        value="$default_value"
    fi
    
    eval "$var_name=\"$value\""
}

# Show usage
usage() {
    echo "Usage: $SCRIPT_NAME [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --upgrade <path>   Upgrade existing installation at <path>"
    echo "  --help             Show this help message"
    echo ""
    echo "Without options, performs fresh installation in current directory."
    exit 0
}

# Upgrade existing installation
do_upgrade() {
    local target="$1"
    local copied_missing=""
    local bin_count=0
    local lib_count=0
    
    clear
    display_logo
    
    section "Upgrade Mode"
    
    echo -e "${CYAN}Source:${NC}      ${WHITE}$BASE_DIR${NC}"
    echo -e "${CYAN}Target:${NC}      ${WHITE}$target${NC}"
    echo ""
    
    # Verify target is a valid Maximus installation
    if [ ! -f "$target/etc/max.ctl" ] && [ ! -f "$target/etc/max.prm" ]; then
        log_error "Target does not appear to be a Maximus installation"
        log_error "Missing etc/max.ctl or etc/max.prm"
        exit 1
    fi
    
    log_ok "Valid Maximus installation detected"
    echo ""
    
    # Confirm
    echo -ne "${YELLOW}Continue with upgrade?${NC} [${WHITE}Y/n${NC}]: "
    read confirm
    if [ "$confirm" = "n" ] || [ "$confirm" = "N" ]; then
        echo ""
        log_warn "Upgrade cancelled."
        exit 0
    fi
    
    section "Copying Executables and Libraries"
    
    # Copy bin/ files
    log_info "Copying bin/ files..."
    for f in bin/*; do
        if [ -f "$f" ]; then
            cp -f "$f" "$target/bin/"
            bin_count=$((bin_count + 1))
        fi
    done
    log_ok "Copied $bin_count files to bin/"
    
    # Copy lib/ files
    log_info "Copying lib/ files..."
    for f in lib/*; do
        if [ -f "$f" ]; then
            cp -f "$f" "$target/lib/"
            lib_count=$((lib_count + 1))
        fi
    done
    log_ok "Copied $lib_count files to lib/"
    
    # On macOS, clear quarantine and codesign copied binaries
    if [ "$(uname -s)" = "Darwin" ]; then
        log_info "Clearing quarantine and codesigning (macOS)..."
        xattr -cr "$target/bin" "$target/lib" 2>/dev/null || true
        for bin in "$target/bin/"*; do
            if [ -f "$bin" ] && file "$bin" | grep -q "Mach-O"; then
                codesign --force --sign - "$bin" 2>/dev/null || true
            fi
        done
        for lib in "$target/lib/"*.so "$target/lib/"*.dylib; do
            if [ -f "$lib" ]; then
                codesign --force --sign - "$lib" 2>/dev/null || true
            fi
        done
        log_ok "Binaries signed"
    fi
    
    # Copy man/ files
    log_info "Copying man/ files..."
    if [ -d "man" ]; then
        [ -d "$target/man" ] || mkdir -p "$target/man"
        for f in man/*; do
            if [ -f "$f" ]; then
                cp -f "$f" "$target/man/"
            fi
        done
        log_ok "Copied man pages"
    fi
    
    section "Checking for Missing Files"
    
    # Helper function to check directory for missing files
    check_missing_files() {
        local src_dir="$1"
        local dir_name="$2"
        
        if [ -d "$src_dir" ]; then
            log_info "Checking $dir_name/..."
            [ -d "$target/$src_dir" ] || mkdir -p "$target/$src_dir"
            for f in "$src_dir"/*; do
                if [ -f "$f" ]; then
                    fname="$(basename "$f")"
                    if [ ! -f "$target/$src_dir/$fname" ]; then
                        cp -f "$f" "$target/$src_dir/"
                        copied_missing="$copied_missing\n  ${LGREEN}+${NC} $src_dir/$fname"
                    fi
                fi
            done
        fi
    }
    
    # Check m/ directory
    check_missing_files "m" "MEX scripts"
    
    # Check all etc/ subdirectories
    for subdir in etc/*/; do
        if [ -d "$subdir" ]; then
            subdir_name="${subdir%/}"  # Remove trailing slash
            check_missing_files "$subdir_name" "$subdir_name"
        fi
    done
    
    section "Upgrade Complete"
    
    echo ""
    echo -e "${LGREEN}Upgrade completed successfully!${NC}"
    echo ""
    echo -e "${WHITE}Summary:${NC}"
    echo -e "  ${CYAN}bin/ files:${NC} $bin_count copied"
    echo -e "  ${CYAN}lib/ files:${NC} $lib_count copied"
    
    if [ -n "$copied_missing" ]; then
        echo ""
        echo -e "${WHITE}New files added:${NC}"
        echo -e "$copied_missing"
    else
        echo ""
        echo -e "  ${CYAN}No missing files detected${NC}"
    fi
    
    echo ""
    echo -e "${YELLOW}Note:${NC} Configuration files in etc/ were NOT modified."
    echo -e "      Run ${WHITE}bin/recompile.sh${NC} in target if needed."
    echo ""
    echo -e "${LBLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${WHITE}               Upgrade complete. Happy BBS'ing!${NC}"
    echo -e "${LBLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
}

# Main installation
main() {
    clear
    display_logo
    
    section "Installation Configuration"
    
    INSTALL_PATH="$(pwd)"
    echo -e "${CYAN}Installation path:${NC} ${WHITE}$INSTALL_PATH${NC}"
    echo ""
    
    # Get BBS configuration
    prompt BBS_NAME "Enter your BBS name" "My Maximus BBS"
    prompt SYSOP_NAME "Enter sysop name" "Sysop"
    
    echo ""
    log_info "Configuring Maximus BBS..."
    echo -e "  ${CYAN}BBS Name:${NC}  $BBS_NAME"
    echo -e "  ${CYAN}Sysop:${NC}     $SYSOP_NAME"
    echo -e "  ${CYAN}Path:${NC}      $INSTALL_PATH"
    echo ""
    
    # Confirm
    echo -ne "${YELLOW}Continue with installation?${NC} [${WHITE}Y/n${NC}]: "
    read confirm
    if [ "$confirm" = "n" ] || [ "$confirm" = "N" ]; then
        echo ""
        log_warn "Installation cancelled."
        exit 0
    fi
    
    section "Updating Configuration Files"
    
    # Update paths in max.ctl - replace ./ with absolute path
    log_info "Setting absolute paths in etc/max.ctl..."
    
    # Backup original
    cp etc/max.ctl etc/max.ctl.bak
    
    # Replace Path System . with absolute path
    sed -i.tmp "s|^[[:space:]]*Path System[[:space:]]*\.|        Path System     $INSTALL_PATH|" etc/max.ctl
    
    # Replace other ./ prefixed paths  
    sed -i.tmp "s|\./|$INSTALL_PATH/|g" etc/max.ctl
    
    # Update BBS name (line 47: Name Battle Station BBS [EXAMPLE])
    sed -i.tmp "s|^[[:space:]]*Name[[:space:]]*Battle Station BBS \[EXAMPLE\]|        Name            $BBS_NAME|" etc/max.ctl
    
    # Update sysop name (line 58: SysOp Limping Ninja [EXAMPLE])
    sed -i.tmp "s|^[[:space:]]*SysOp[[:space:]]*Limping Ninja \[EXAMPLE\]|        SysOp           $SYSOP_NAME|" etc/max.ctl
    
    # Clean up temp files
    rm -f etc/max.ctl.tmp
    
    log_ok "Configuration updated"
    
    # Update other config files that might have paths
    for cfg in etc/squish.cfg etc/compress.cfg etc/areas.bbs; do
        if [ -f "$cfg" ]; then
            log_info "Updating $cfg..."
            sed -i.tmp "s|\./|$INSTALL_PATH/|g" "$cfg"
            rm -f "$cfg.tmp"
        fi
    done
    
    section "Compiling Configuration"
    
    log_info "Running recompile.sh..."
    echo ""
    
    if [ -x bin/recompile.sh ]; then
        bin/recompile.sh
    else
        log_error "recompile.sh not found or not executable"
        exit 1
    fi
    
    section "Installation Complete"
    
    echo ""
    echo -e "${LGREEN}Maximus BBS has been configured!${NC}"
    echo ""
    echo -e "${WHITE}Quick Start:${NC}"
    echo ""
    echo -e "  ${CYAN}Local console mode:${NC}"
    echo -e "    ${WHITE}bin/runbbs.sh -c${NC}"
    echo ""
    echo -e "  ${CYAN}Telnet server (MAXTEL):${NC}"
    echo -e "    ${WHITE}bin/maxtel -p 2323 -n 4${NC}"
    echo -e "    Then connect: ${WHITE}telnet localhost 2323${NC}"
    echo ""
    echo -e "  ${CYAN}Headless/daemon mode:${NC}"
    echo -e "    ${WHITE}bin/maxtel -H -p 2323 -n 4${NC}  (headless)"
    echo -e "    ${WHITE}bin/maxtel -D -p 2323 -n 4${NC}  (daemon)"
    echo ""
    echo -e "${CYAN}Configuration files are in:${NC} ${WHITE}etc/${NC}"
    echo -e "${CYAN}After editing .ctl files, run:${NC} ${WHITE}bin/recompile.sh${NC}"
    echo ""
    echo -e "${LBLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${WHITE}        Welcome to Maximus! Enjoy your BBS journey.${NC}"
    echo -e "${LBLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
}

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        --upgrade)
            UPGRADE_MODE=1
            if [ -z "$2" ] || [ "${2:0:1}" = "-" ]; then
                log_error "--upgrade requires a path argument"
                exit 1
            fi
            UPGRADE_PATH="$2"
            shift 2
            ;;
        --help|-h)
            usage
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Run appropriate mode
if [ "$UPGRADE_MODE" = "1" ]; then
    do_upgrade "$UPGRADE_PATH"
else
    main
fi
