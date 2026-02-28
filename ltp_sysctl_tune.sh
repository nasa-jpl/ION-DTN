#!/usr/bin/env bash
#
# ltp_sysctl_tune.sh - Toggle Linux UDP buffer sysctl settings for
# high-throughput LTP performance.
#
# Usage:
#   sudo ./ltp_sysctl_tune.sh          # Show current settings
#   sudo ./ltp_sysctl_tune.sh on       # Apply high-throughput settings
#   sudo ./ltp_sysctl_tune.sh off      # Restore Linux defaults
#   sudo ./ltp_sysctl_tune.sh persist  # Install persistent config file
#   sudo ./ltp_sysctl_tune.sh unpersist # Remove persistent config file
#
# See gh-pages/docs/topics/ltp-udp-multisend-tuning.md for details.

set -e

# High-throughput values
RMEM_MAX=16777216       # 16 MB
WMEM_MAX=16777216       # 16 MB
RMEM_DEFAULT=8388608    #  8 MB
WMEM_DEFAULT=8388608    #  8 MB

# Linux defaults
RMEM_MAX_DEFAULT=212992
WMEM_MAX_DEFAULT=212992
RMEM_DEFAULT_DEFAULT=212992
WMEM_DEFAULT_DEFAULT=212992

PERSIST_FILE="/etc/sysctl.d/90-ion-ltp.conf"

PARAMS=(
    net.core.rmem_max
    net.core.wmem_max
    net.core.rmem_default
    net.core.wmem_default
)

show_current() {
    echo "Current UDP buffer sysctl settings:"
    echo "------------------------------------"
    for p in "${PARAMS[@]}"; do
        val=$(sysctl -n "$p" 2>/dev/null || echo "N/A")
        printf "  %-28s = %s\n" "$p" "$val"
    done
    if [ -f "$PERSIST_FILE" ]; then
        echo ""
        echo "Persistent config: $PERSIST_FILE (installed)"
    fi
}

apply_values() {
    local rmem_max=$1 wmem_max=$2 rmem_def=$3 wmem_def=$4

    sysctl -w net.core.rmem_max="$rmem_max"
    sysctl -w net.core.wmem_max="$wmem_max"
    sysctl -w net.core.rmem_default="$rmem_def"
    sysctl -w net.core.wmem_default="$wmem_def"
}

case "$(uname -s)" in
    Linux) ;;
    *)
        echo "Error: This script is for Linux only." >&2
        exit 1
        ;;
esac

case "${1:-}" in
    on)
        echo "Applying high-throughput LTP sysctl settings..."
        apply_values $RMEM_MAX $WMEM_MAX $RMEM_DEFAULT $WMEM_DEFAULT
        echo ""
        show_current
        echo ""
        echo "Settings applied (will be lost on reboot)."
        echo "Use '$0 persist' to make them permanent."
        ;;

    off)
        echo "Restoring Linux default sysctl settings..."
        apply_values $RMEM_MAX_DEFAULT $WMEM_MAX_DEFAULT \
                     $RMEM_DEFAULT_DEFAULT $WMEM_DEFAULT_DEFAULT
        echo ""
        show_current
        echo ""
        echo "Defaults restored (will be lost on reboot if persistent config exists)."
        echo "Use '$0 unpersist' to also remove the persistent config."
        ;;

    persist)
        echo "Installing persistent config to $PERSIST_FILE..."
        cat > "$PERSIST_FILE" <<EOF
# ION LTP high-throughput UDP buffer settings
# Installed by ltp_sysctl_tune.sh
# See gh-pages/docs/topics/ltp-udp-multisend-tuning.md
net.core.rmem_max=$RMEM_MAX
net.core.wmem_max=$WMEM_MAX
net.core.rmem_default=$RMEM_DEFAULT
net.core.wmem_default=$WMEM_DEFAULT
EOF
        sysctl -p "$PERSIST_FILE"
        echo ""
        show_current
        echo ""
        echo "Persistent config installed. Settings will survive reboot."
        ;;

    unpersist)
        if [ -f "$PERSIST_FILE" ]; then
            rm -f "$PERSIST_FILE"
            echo "Removed $PERSIST_FILE."
            echo "Run '$0 off' to also restore defaults in the running kernel."
        else
            echo "No persistent config found at $PERSIST_FILE."
        fi
        ;;

    ""|status)
        show_current
        echo ""
        echo "Usage: $0 {on|off|persist|unpersist}"
        echo "  on        - Apply high-throughput settings (temporary)"
        echo "  off       - Restore Linux defaults (temporary)"
        echo "  persist   - Install persistent config to $PERSIST_FILE"
        echo "  unpersist - Remove persistent config file"
        ;;

    *)
        echo "Unknown option: $1" >&2
        echo "Usage: $0 {on|off|persist|unpersist}" >&2
        exit 1
        ;;
esac
