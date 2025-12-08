#!/bin/bash
# Restore system settings after benchmarking

source "$(dirname "$0")/common.sh"

[[ -f "$CONFIG_FILE" ]] || { log "No config found. Nothing to restore."; exit 0; }

if [[ $EUID -eq 0 ]]; then
    log "Restoring ASLR"
    restore_aslr
    log "Done. CPU governor persists until reboot."
else
    warn "Run with sudo to restore ASLR"
fi
