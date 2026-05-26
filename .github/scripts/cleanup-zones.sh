#!/bin/env bash
# Manual backup cleanup script for Solaris zones created by CI
# Usage: ./cleanup-zones.sh
#   - Takes no inputs.
#   - Cleans ALL ci-zone-* zones unconditionally.

set -e

ZONE_PATTERN="ci-zone-"
echo "Starting full recovery cleanup for all zones matching '${ZONE_PATTERN}'..."

for HOST in dsoc3 dsoc4; do
    echo ""
    echo "=== Cleaning zones on $HOST ==="

    # Get zones in ALL states (configured, installed, incomplete, etc)
    # Using zoneadm -p (parseable) with cut avoids Solaris awk escaping syntax errors
    ZONES=$(ssh -o StrictHostKeyChecking=no github-runner@$HOST "
        sudo zoneadm list -pc | cut -d: -f2 | grep '^${ZONE_PATTERN}' || true
    ")

    if [ -n "$ZONES" ]; then
        ZONE_COUNT=$(echo "$ZONES" | wc -w)
        echo "Found $ZONE_COUNT zone(s) to clean"

        while IFS= read -r zone; do
            if [ -n "$zone" ]; then
                echo "Destroying zone: $zone"
                # ssh -n prevents ssh from consuming the rest of the while loop's stdin
                ssh -n -o StrictHostKeyChecking=no github-runner@$HOST "
                    # Force halt if running
                    sudo zoneadm -z $zone halt 2>/dev/null || true

                    # Uninstall (removes ZFS datasets automatically if managed by zone)
                    for i in 1 2 3; do
                        sudo zoneadm -z $zone uninstall -F 2>/dev/null && break
                        sleep 1
                    done

                    # Delete zone config
                    sudo zonecfg -z $zone delete -F 2>/dev/null || true
                " 2>&1 | sed 's/^/  /'
            fi
        done <<<"$ZONES"
    else
        echo "No zones found matching pattern."
    fi

    ZFS_DATASETS=$(ssh -o StrictHostKeyChecking=no github-runner@$HOST "
        sudo zfs list -H -o name | grep 'rpool/zones/${ZONE_PATTERN}' | grep -v '@' || true
    ")

    if [ -n "$ZFS_DATASETS" ]; then
        echo ""
        echo "Cleaning remaining ZFS datasets (guaranteed cleanup)..."

        while IFS= read -r dataset; do
            if [ -n "$dataset" ]; then
                echo "  Destroying: $dataset"
                # ssh -n prevents ssh from consuming the rest of the while loop's stdin
                ssh -n -o StrictHostKeyChecking=no github-runner@$HOST "
                    sudo zfs destroy -r '$dataset' 2>/dev/null || \
                    sudo zfs destroy -rf '$dataset' 2>/dev/null || \
                    echo '    Failed to destroy $dataset'
                "
            fi
        done <<<"$ZFS_DATASETS"
    else
        echo "No orphaned ZFS datasets found."
    fi

    echo "✓ Cleanup complete on $HOST"
done

echo ""
echo "=== Cleanup Summary ==="
for HOST in dsoc3 dsoc4; do
    ZONES_REMAINING=$(ssh -o StrictHostKeyChecking=no github-runner@$HOST "
        sudo zoneadm list -pc | cut -d: -f2 | grep '^${ZONE_PATTERN}' || true
    ")
    ZFS_REMAINING=$(ssh -o StrictHostKeyChecking=no github-runner@$HOST "
        sudo zfs list -H -o name | grep 'rpool/zones/${ZONE_PATTERN}' | grep -v '@' || true
    ")

    ZONES_REMAINING=$(echo "$ZONES_REMAINING" | wc -w)
    ZFS_REMAINING=$(echo "$ZFS_REMAINING" | wc -w)

    echo "$HOST: $ZONES_REMAINING zone(s), $ZFS_REMAINING dataset(s) remaining"

    if [ "$ZONES_REMAINING" -gt 0 ] || [ "$ZFS_REMAINING" -gt 0 ]; then
        echo "  Remaining items:"
        ssh -o StrictHostKeyChecking=no github-runner@$HOST "
            sudo zoneadm list -pc | cut -d: -f2 | grep '^${ZONE_PATTERN}' || true
            sudo zfs list -H -o name | grep 'rpool/zones/${ZONE_PATTERN}' | grep -v '@' || true
        " | sed 's/^/    /'
    fi
done
