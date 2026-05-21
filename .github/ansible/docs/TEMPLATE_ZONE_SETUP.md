# Template Zone Setup Guide

This guide explains how to create the template-ion zone on dsoc3 and dsoc4 for CI test parallelization.

## Prerequisites

- Root or sudo access on dsoc3 and dsoc4
- ZFS filesystem at `/zones` or `rpool/zones`
- **No network configuration needed** (uses shared IP mode)
- Hostnames dsoc3 and dsoc4 must be configured in the runner's /etc/hosts file

## Overview

The template zone (`template-ion`) is a pre-configured Solaris Zone containing all ION build dependencies. Test zones are created by cloning ZFS snapshots of this template, which is much faster than installing from scratch.

## Step-by-Step Setup

### 1. Create ZFS dataset for zones

```bash
# On dsoc3 and dsoc4
sudo zfs create -o mountpoint=/zones rpool/zones
sudo zfs create -o mountpoint=/zones/template-ion rpool/zones/template-ion
```

### 2. Configure template zone

```bash
sudo zonecfg -z template-ion <<'EOF'
create
set ip-type=shared
remove anet
add net
set physical=net0
set address=192.168.1.238/24
end
set zonepath=/zones/template-ion
set autoboot=false
EOF
```

### 3. Install template zone

```bash
sudo zoneadm -z template-ion install
```

This may take several minutes for the initial installation.

### 4. Boot template zone

```bash
sudo zoneadm -z template-ion boot
```

Wait 30-60 seconds for zone to fully boot.

### 5. Install ION build dependencies

```bash
# Install required packages in template zone
sudo zlogin template-ion pkg install gcc gmake autoconf automake libtool

# Install ION-specific dependencies
sudo zlogin template-ion pkg install pkg:/system/library/math/header-math
sudo zlogin template-ion pkg install pkg:/library/security/openssl

# If mbedtls is needed
sudo zlogin template-ion pkg install mbedtls

# Verify installations
sudo zlogin template-ion which gcc gmake autoconf automake libtool
```

### 6. Set up Python virtual environment

```bash
# Create Python virtual environment for test dependencies
sudo zlogin template-ion 'python3 -m venv /root/ion_dev'

# Activate venv and install required packages
sudo zlogin template-ion 'source /root/ion_dev/bin/activate && pip install --upgrade pip'

# Install any Python packages needed for ION tests
# Example: sudo zlogin template-ion 'source /root/ion_dev/bin/activate && pip install pytest pytest-timeout'
```

**Note:** The test execution scripts automatically activate this venv at `/root/ion_dev` before running tests.

### 7. Configure zone environment

```bash
# Set up any required environment variables or configurations
sudo zlogin template-ion 'echo "export PATH=/usr/local/bin:\$PATH" >> /root/.profile'
```

### 8. Halt template zone

```bash
sudo zoneadm -z template-ion halt
```

### 9. Create ZFS snapshot

```bash
sudo zfs snapshot rpool/zones/template-ion@ci-base
```

### 10. Verify snapshot exists

```bash
zfs list -t snapshot | grep template-ion
```

Expected output:

```
rpool/zones/template-ion@ci-base  [size]  -  [date]  -
```

## Verification

Test that zone cloning works:

```bash
# Configure and boot test zone
sudo zonecfg -z test-clone <<EOF
create
remove anet
set zonepath=/zones/test-clone
set autoboot=false
set ip-type=shared
EOF

sudo zoneadm -z test-clone clone template-ion
sudo zoneadm -z test-clone boot

# Verify it works
sleep 10
sudo zlogin test-clone gcc --version

# Cleanup test zone
sudo zoneadm -z test-clone halt
sudo zoneadm -z test-clone uninstall -F
sudo zonecfg -z test-clone delete -F
```

If you see gcc version output, the template is working correctly!

## Updating the Template

To update dependencies in the template zone:

1. Boot the template zone
2. Make your changes (install packages, update configs)
3. Halt the zone
4. Create a new snapshot with a different name (e.g., `@ci-base-v2`)
5. Update `.github/ansible/group_vars/all.yml` to point to the new snapshot

## Troubleshooting

**"failed to add network device" error:**
This means the zone was configured with `ip-type=exclusive` but the network interface doesn't exist.

Solution - Switch to shared IP mode:

```bash
# Delete the zone (if it exists)
sudo zoneadm -z template-ion halt 2>/dev/null
sudo zoneadm -z template-ion uninstall -F 2>/dev/null
sudo zonecfg -z template-ion delete -F 2>/dev/null

# Recreate with shared IP (no network interface needed)
sudo zonecfg -z template-ion <<EOF
create
set zonepath=/zones/template-ion
set autoboot=false
set ip-type=shared
EOF

# Continue with installation
sudo zoneadm -z template-ion install
```

**Zone won't boot:**

- Check `sudo zoneadm -z template-ion list -v` for status
- Review zone logs: `sudo cat /var/log/zones/zoneadm.*`

**Package installation fails:**

- Verify network connectivity from within zone: `sudo zlogin template-ion ping 8.8.8.8`
- Check zone's pkg publisher: `sudo zlogin template-ion pkg publisher`

**Snapshot creation fails:**

- Ensure zone is halted: `sudo zoneadm -z template-ion list`
- Check ZFS space: `zfs list -o space rpool/zones`

**Clone is slow:**

- Verify you're using ZFS clone, not copy
- Check that origin snapshot exists: `zfs get origin rpool/zones/[clone-name]`

## Advanced: Using Exclusive IP Mode

**Only use exclusive IP mode if you need network isolation.** Most CI environments work fine with shared IP mode.

If you need exclusive mode:

1. **Find available network interfaces:**

   ```bash
   dladm show-link
   # Look for active physical interfaces like: e1000g0, igb0, net0, etc.
   ```

2. **Create a VNIC (Virtual NIC):**

   ```bash
   # Replace 'e1000g0' with your actual physical interface name
   sudo dladm create-vnic -l e1000g0 vnic0

   # Verify it was created
   dladm show-vnic
   ```

3. **Configure zone with the VNIC:**

   ```bash
   sudo zonecfg -z template-ion <<EOF
   create
   set zonepath=/zones/template-ion
   set autoboot=false
   set ip-type=exclusive
   add net
   set physical=vnic0
   end
   EOF
   ```

4. **Update Ansible configuration:**

   ```yaml
   # In .github/ansible/group_vars/all.yml
   zone_ip_type: "exclusive"
   zone_physical_interface: "vnic0"
   ```

## Security Considerations

- Template zone should contain NO sensitive data (credentials, keys, etc.)
- Install only the minimum required packages
- Keep template zone halted when not in use
- Regularly update template zone packages for security patches

## Maintenance

Recommended maintenance schedule:

- Monthly: Update packages in template zone
- Quarterly: Review and update ION dependencies
- After major ION releases: Rebuild template with new dependencies
