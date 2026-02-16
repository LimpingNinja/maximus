#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# install.sh - Maximus BBS Installation Script (TOML-based)
#
# Copyright (C) 2025 Kevin Morgan (Limping Ninja)
# https://github.com/LimpingNinja
#
# This script performs a fresh Maximus installation by:
# - Ensuring all required runtime directories exist
# - Setting sys_path in maximus.toml
# - Optionally configuring BBS name and sysop name
# - Initializing the SQLite user database
# - Validating required configuration and display files
#

set -e

# ── Colors ────────────────────────────────────────────────────────────────────
CYAN='\033[0;36m'
LCYAN='\033[1;36m'
YELLOW='\033[1;33m'
WHITE='\033[1;37m'
GREEN='\033[0;32m'
LGREEN='\033[1;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
LBLUE='\033[1;34m'
NC='\033[0m'

# ── Path discovery ────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_NAME="$(basename "$0")"

if [ "$(basename "$SCRIPT_DIR")" = "bin" ]; then
    BASE_DIR="$(dirname "$SCRIPT_DIR")"
else
    echo -e "${RED}Error:${NC} Run from the installation root:  bin/$SCRIPT_NAME"
    exit 1
fi

cd "$BASE_DIR"

# ── Helpers ───────────────────────────────────────────────────────────────────
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
    echo -e "${CYAN}                   Installation Script (NG)${NC}"
    echo ""
}

section() {
    echo ""
    echo -e "${LCYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${WHITE}  $1${NC}"
    echo -e "${LCYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

log_info() { echo -e "  ${CYAN}[INFO]${NC}  $1"; }
log_ok()   { echo -e "  ${LGREEN}[ OK ]${NC}  $1"; }
log_warn() { echo -e "  ${YELLOW}[WARN]${NC}  $1"; }
log_fail() { echo -e "  ${RED}[FAIL]${NC}  $1"; }

prompt() {
    local var_name="$1" prompt_text="$2" default_value="$3" value
    if [ -n "$default_value" ]; then
        echo -ne "  ${YELLOW}$prompt_text${NC} [${WHITE}$default_value${NC}]: "
    else
        echo -ne "  ${YELLOW}$prompt_text${NC}: "
    fi
    read -r value
    [ -z "$value" ] && value="$default_value"
    eval "$var_name=\"\$value\""
}

# ── Usage ─────────────────────────────────────────────────────────────────────
usage() {
    echo "Usage: $SCRIPT_NAME [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --validate   Run validation checks only (no changes)"
    echo "  --help       Show this help message"
    echo ""
    echo "Without options, performs fresh installation in current directory."
    exit 0
}

# ── Directory structure ───────────────────────────────────────────────────────
# Required directories that must exist at runtime.  The install script creates
# them; at runtime, IPC node dirs and Squish bases are auto-created by code.
REQUIRED_DIRS=(
    config
    config/general
    config/lang
    config/menus
    config/areas/msg
    config/areas/file
    config/security
    display/screens
    display/help
    display/tunes
    scripts
    scripts/include
    data/users
    data/db
    data/msgbase
    data/filebase
    data/files
    data/nodelist
    data/mail/inbound
    data/mail/outbound
    data/mail/netmail
    data/mail/attach
    data/mail/bad
    data/mail/dupes
    data/olr
    run/tmp
    run/node
    run/stage
    log
    doors
)

# Required TOML config files that must be present for boot
REQUIRED_CONFIGS=(
    config/maximus.toml
    config/general/session.toml
    config/general/display_files.toml
    config/general/equipment.toml
    config/general/colors.toml
    config/general/reader.toml
    config/general/protocol.toml
    config/general/language.toml
    config/security/access_levels.toml
    config/areas/msg/areas.toml
    config/areas/file/areas.toml
    config/matrix.toml
    config/lang/english.toml
)

# Required menu TOMLs
REQUIRED_MENUS=(
    config/menus/main.toml
    config/menus/message.toml
    config/menus/file.toml
    config/menus/reader.toml
    config/menus/change.toml
    config/menus/chat.toml
    config/menus/edit.toml
    config/menus/sysop.toml
)

# ── Ensure directories ────────────────────────────────────────────────────────
ensure_dirs() {
    local created=0 existed=0
    for d in "${REQUIRED_DIRS[@]}"; do
        if [ ! -d "$d" ]; then
            mkdir -p "$d"
            created=$((created + 1))
        else
            existed=$((existed + 1))
        fi
    done
    log_ok "Directories: $existed existed, $created created"
}

# ── Validate configs ──────────────────────────────────────────────────────────
validate_configs() {
    local missing=0 ok=0
    for f in "${REQUIRED_CONFIGS[@]}" "${REQUIRED_MENUS[@]}"; do
        if [ ! -f "$f" ]; then
            log_fail "Missing: $f"
            missing=$((missing + 1))
        else
            ok=$((ok + 1))
        fi
    done
    if [ "$missing" -gt 0 ]; then
        log_warn "$missing config file(s) missing — BBS may not boot correctly"
        return 1
    else
        log_ok "All $ok required config files present"
        return 0
    fi
}

# ── Validate binaries ─────────────────────────────────────────────────────────
validate_binaries() {
    local missing=0
    for b in max maxtel mex mecca init_userdb; do
        if [ ! -x "bin/$b" ]; then
            log_fail "Missing binary: bin/$b"
            missing=$((missing + 1))
        fi
    done
    if [ "$missing" -gt 0 ]; then
        log_warn "$missing binary(ies) missing — run 'make install' first"
        return 1
    else
        log_ok "All required binaries present"
        return 0
    fi
}

# ── Initialize user database ─────────────────────────────────────────────────
init_userdb() {
    local db_path="data/users/user.db"
    local schema_path="data/db/userdb_schema.sql"

    if [ -f "$db_path" ]; then
        log_ok "User database already exists: $db_path"
        return 0
    fi

    if [ ! -f "$schema_path" ]; then
        log_fail "Schema not found: $schema_path"
        return 1
    fi

    # Prefer the compiled init_userdb helper
    if [ -x "bin/init_userdb" ]; then
        log_info "Initializing user database via init_userdb..."
        if bin/init_userdb --prefix "$BASE_DIR" --db "$db_path" --schema "$schema_path"; then
            log_ok "User database created: $db_path"
            return 0
        else
            log_fail "init_userdb failed"
            return 1
        fi
    fi

    # Fallback to sqlite3 CLI
    if command -v sqlite3 >/dev/null 2>&1; then
        log_info "Initializing user database via sqlite3 CLI..."
        mkdir -p "$(dirname "$db_path")"
        if sqlite3 "$db_path" < "$schema_path"; then
            log_ok "User database created: $db_path"
            return 0
        else
            log_fail "sqlite3 schema apply failed"
            rm -f "$db_path"
            return 1
        fi
    fi

    log_fail "No init_userdb binary or sqlite3 CLI available"
    return 1
}

# ── Update sys_path in maximus.toml ──────────────────────────────────────────
update_sys_path() {
    local toml="config/maximus.toml"
    if [ ! -f "$toml" ]; then
        log_fail "Missing $toml — cannot set sys_path"
        return 1
    fi

    # Replace sys_path value with the resolved absolute path
    if grep -q '^sys_path' "$toml"; then
        sed -i.bak "s|^sys_path[[:space:]]*=.*|sys_path = \"$BASE_DIR\"|" "$toml"
        rm -f "$toml.bak"
        log_ok "sys_path set to $BASE_DIR"
    else
        log_warn "No sys_path key found in $toml — adding it"
        echo "sys_path = \"$BASE_DIR\"" >> "$toml"
        log_ok "sys_path appended"
    fi
}

# ── Update BBS name / sysop in maximus.toml ──────────────────────────────────
update_bbs_identity() {
    local toml="config/maximus.toml"
    local bbs_name="$1"
    local sysop_name="$2"

    if [ -n "$bbs_name" ] && grep -q '^system_name' "$toml"; then
        sed -i.bak "s|^system_name[[:space:]]*=.*|system_name = \"$bbs_name\"|" "$toml"
        rm -f "$toml.bak"
        log_ok "BBS name set to: $bbs_name"
    fi

    if [ -n "$sysop_name" ] && grep -q '^sysop' "$toml"; then
        sed -i.bak "s|^sysop[[:space:]]*=.*|sysop = \"$sysop_name\"|" "$toml"
        rm -f "$toml.bak"
        log_ok "Sysop name set to: $sysop_name"
    fi
}

# ── macOS codesigning ─────────────────────────────────────────────────────────
codesign_if_macos() {
    if [ "$(uname -s)" != "Darwin" ]; then
        return 0
    fi
    log_info "Codesigning binaries (macOS)..."
    xattr -cr bin/ lib/ 2>/dev/null || true
    for f in bin/*; do
        [ -f "$f" ] && file "$f" | grep -q "Mach-O" && codesign --force --sign - "$f" 2>/dev/null || true
    done
    for f in lib/*.so lib/*.dylib bin/lib/*.so bin/lib/*.dylib; do
        [ -f "$f" ] && codesign --force --sign - "$f" 2>/dev/null || true
    done
    log_ok "Binaries signed"
}

# ── Validate-only mode ────────────────────────────────────────────────────────
do_validate() {
    display_logo
    section "Validation"

    echo -e "  ${CYAN}Install path:${NC} ${WHITE}$BASE_DIR${NC}"
    echo ""

    local errors=0

    validate_binaries  || errors=$((errors + 1))
    validate_configs   || errors=$((errors + 1))

    # Check user DB
    if [ -f "data/users/user.db" ]; then
        log_ok "User database exists"
    else
        log_warn "User database missing — run install to create"
        errors=$((errors + 1))
    fi

    # Check schema
    if [ -f "data/db/userdb_schema.sql" ]; then
        log_ok "User DB schema present"
    else
        log_fail "Missing data/db/userdb_schema.sql"
        errors=$((errors + 1))
    fi

    # Check display files
    local dsp_count
    dsp_count=$(find display/screens -type f 2>/dev/null | wc -l | tr -d ' ')
    if [ "$dsp_count" -gt 0 ]; then
        log_ok "Display screens: $dsp_count files"
    else
        log_warn "No display files in display/screens/"
    fi

    # Check MEX scripts
    local mex_count
    mex_count=$(find scripts -name '*.vm' -type f 2>/dev/null | wc -l | tr -d ' ')
    if [ "$mex_count" -gt 0 ]; then
        log_ok "MEX bytecode: $mex_count .vm files"
    else
        log_warn "No compiled MEX scripts (*.vm) in scripts/"
    fi

    echo ""
    if [ "$errors" -gt 0 ]; then
        log_warn "Validation found $errors issue(s)"
        return 1
    else
        log_ok "All checks passed"
        return 0
    fi
}

# ── Main installation ─────────────────────────────────────────────────────────
main() {
    clear
    display_logo

    # Quick sanity: must have config/ dir (not legacy etc/)
    if [ ! -d "config" ]; then
        log_fail "No config/ directory found."
        log_fail "This does not appear to be a Maximus NG installation."
        exit 1
    fi

    section "Installation Configuration"

    echo -e "  ${CYAN}Installation path:${NC} ${WHITE}$BASE_DIR${NC}"
    echo ""

    prompt BBS_NAME  "Enter your BBS name" "My Maximus BBS"
    prompt SYSOP_NAME "Enter sysop name"   "Sysop"

    echo ""
    log_info "Configuring Maximus BBS..."
    echo -e "    ${CYAN}BBS Name:${NC}  $BBS_NAME"
    echo -e "    ${CYAN}Sysop:${NC}     $SYSOP_NAME"
    echo -e "    ${CYAN}Path:${NC}      $BASE_DIR"
    echo ""

    echo -ne "  ${YELLOW}Continue with installation?${NC} [${WHITE}Y/n${NC}]: "
    read -r confirm
    if [ "$confirm" = "n" ] || [ "$confirm" = "N" ]; then
        echo ""
        log_warn "Installation cancelled."
        exit 0
    fi

    # ── 1. Directories ────────────────────────────────────────────────────
    section "Ensuring Directory Structure"
    ensure_dirs

    # ── 2. Validate binaries ──────────────────────────────────────────────
    section "Checking Binaries"
    validate_binaries || true

    # ── 3. macOS codesigning ──────────────────────────────────────────────
    codesign_if_macos

    # ── 4. Configure TOML ─────────────────────────────────────────────────
    section "Updating Configuration"
    update_sys_path
    update_bbs_identity "$BBS_NAME" "$SYSOP_NAME"

    # ── 5. Initialize user database ───────────────────────────────────────
    section "User Database"
    init_userdb || true

    # ── 6. Validate configs ───────────────────────────────────────────────
    section "Validating Configuration Files"
    validate_configs || true

    # ── 7. Summary ────────────────────────────────────────────────────────
    section "Installation Complete"

    echo ""
    echo -e "  ${LGREEN}Maximus BBS has been configured!${NC}"
    echo ""
    echo -e "  ${WHITE}Quick Start:${NC}"
    echo ""
    echo -e "    ${CYAN}Telnet server:${NC}"
    echo -e "      ${WHITE}bin/maxtel -p 2323 -n 4${NC}"
    echo -e "      Then connect: ${WHITE}telnet localhost 2323${NC}"
    echo ""
    echo -e "    ${CYAN}Headless / daemon:${NC}"
    echo -e "      ${WHITE}bin/maxtel -H -p 2323 -n 4${NC}"
    echo -e "      ${WHITE}bin/maxtel -D -p 2323 -n 4${NC}"
    echo ""
    echo -e "  ${CYAN}Configuration:${NC}  ${WHITE}config/${NC} (TOML files)"
    echo -e "  ${CYAN}Display files:${NC}  ${WHITE}display/screens/${NC}"
    echo -e "  ${CYAN}MEX scripts:${NC}    ${WHITE}scripts/${NC}"
    echo ""
    echo -e "${LBLUE}  ═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${WHITE}          Welcome to Maximus! Enjoy your BBS journey.${NC}"
    echo -e "${LBLUE}  ═══════════════════════════════════════════════════════════════${NC}"
    echo ""
}

# ── Argument parsing ──────────────────────────────────────────────────────────
VALIDATE_ONLY=0

while [ $# -gt 0 ]; do
    case "$1" in
        --validate)
            VALIDATE_ONLY=1
            shift
            ;;
        --upgrade)
            echo -e "${YELLOW}[WARN]${NC} --upgrade is deprecated and will be reimplemented in a future release."
            exit 0
            ;;
        --help|-h)
            usage
            ;;
        *)
            echo -e "${RED}[ERROR]${NC} Unknown option: $1"
            usage
            ;;
    esac
done

if [ "$VALIDATE_ONLY" = "1" ]; then
    do_validate
else
    main
fi
