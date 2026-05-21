# Solaris Zone Management for Parallel CI Testing

Ansible playbooks for managing ephemeral Solaris Zones for ION test parallelization.

## Architecture

### Overview

The CI workflow uses Solaris Zones to run ION tests in parallel with complete isolation. Each GitHub Actions matrix job gets its own Solaris Zone created from a ZFS snapshot of a template zone.

``` text
GitHub Actions Workflow
  ↓
  1. Build ION (one job, uploads artifact)
  2. Create ALL zones in parallel (create-zones-parallel.yml)
  3. Run tests in parallel (N jobs, each uses pre-created zone)
  4. Upload results (per-job)
  5. Destroy zones (always runs per-job)
  6. Aggregate results
```

### Job Flow

1. **Matrix Generation:** `git_matrix.py` distributes ~129 tests across N runners based on test duration
2. **Build:** One job builds ION, creates tarball, uploads as artifact
3. **Zone Creation (Parallel):** Single job creates ALL zones simultaneously:
   - Uses Ansible's parallelism to create zones on both dsoc3 and dsoc4 at once
   - All zones for dsoc3 created in parallel (async operations)
   - All zones for dsoc4 created in parallel (async operations)
   - Completes in ~15-20 seconds regardless of zone count
4. **Test Execution (Parallel):** N jobs each:
   - Download build artifact
   - Verify pre-created zone is ready
   - Tests run inside isolated zone
   - Results collected with unique names
   - Zone destroyed
5. **Aggregation:** Merge all results and set PR status

### VM Mapping

Two Solaris VMs available:

- **dsoc3**: Build and zone host (hostnames configured in runner's /etc/hosts)
- **dsoc4**: Zone host (hostnames configured in runner's /etc/hosts)

Jobs distribute across VMs using modulo arithmetic:

- `target_vm = dsoc3 if job_index % 2 == 0 else dsoc4`

Multiple jobs can target the same VM safely because each gets an isolated zone.

### Zone Lifecycle

Zones are ephemeral (created and destroyed per test job):

1. **Create:** Clone ZFS snapshot `zones/template-ion@ci-base`
2. **Configure:** Set up exclusive IP networking
3. **Install & Boot:** Zone ready in <30 seconds
4. **Execute:** Install build artifacts, run tests
5. **Collect:** Gather progress files and ion.log files
6. **Destroy:** Remove zone and ZFS clone (guaranteed via Ansible always block)

### Zone Naming

Zones use unique names to prevent collision:

``` JSON
ci-zone-{run_id}-{job_index}
```

Example: `ci-zone-123456789-0` for run 123456789, job index 0

Multiple concurrent CI runs can use the same VMs without conflict.

## Playbooks

### create-zones-parallel.yml

Creates multiple Solaris Zones in parallel from template snapshot.

**Required variables:**

- `run_id`: GitHub Actions run ID
- `zones_for_this_host`: JSON array of zone definitions `[{"job_index": 0, "vm": "dsoc3"}, ...]`

**What it does:**

1. Configures all zones sequentially (fast operation)
2. Generates sysconfig profiles for hostname configuration
3. Clones all zones from template using Ansible async (parallel)
4. Waits for all clone operations to complete
5. Boots all zones using async (parallel)
6. Waits for all boot operations to complete
7. Verifies all zones are accessible via `zlogin`

**Key features:**

- Uses Ansible's `async` with `poll: 0` for true parallelism
- All zones on a VM are created simultaneously
- Significantly faster than sequential creation (15-20s total vs 15-20s per zone)
- All zones created in <30 seconds total
- Zones named `ci-zone-{run_id}-{job_index}`
- Hostnames configured via sysconfig profile during clone

### run-tests-in-zone.sh

Bash script that executes ION tests inside a Solaris Zone using `zlogin`.

**Required arguments:**

- `run_id`: GitHub Actions run ID
- `job_index`: Matrix job index
- `test_list`: Space-separated list of tests to run
- `env_vars`: Environment variables as KEY=VALUE pairs (optional)

**What it does:**

1. Validates all input arguments and checks for command injection
2. Copies build artifact from `/home/github-runner/ci-artifacts/{run_id}/` to zone
3. Extracts tarball inside zone and fixes path references in build files
4. Runs `gmake install` to install ION binaries into `/usr/local`
5. Executes `./runtests {test_list}` inside zone via `zlogin`
6. Collects progress file as `progress-job-{index}.txt`
7. Collects ion.log files with directory structure preserved
8. Collects ion-system.log diagnostic files with flattened naming
9. Returns test exit code

**Deployment:**
The workflow copies this script to the VM via `scp` and executes it with `ssh` and `sudo`.

### destroy-zone.yml

Destroys zone and cleans up resources.

**Required variables:**

- `run_id`: GitHub Actions run ID
- `job_index`: Matrix job index

**What it does:**

1. Halts zone (with force option if needed)
2. Uninstalls zone
3. Destroys ZFS clone
4. Removes zone path directory
5. Cleans up staging directory

Uses `block/rescue/always` to guarantee cleanup even if errors occur.

### copy-code.yml

Synchronizes source code from runner to Solaris VMs.

**Required environment variables:**

- `GITHUB_WORKSPACE`: Path to source code on runner
- `VM_WORK_DIR`: Destination directory on VM

**What it does:**

1. Removes existing work directory
2. Creates clean work directory
3. Synchronizes code using `ansible.posix.synchronize` module
4. Excludes arch-rtems directory

**Replaces:** Direct rsync commands in workflow

### distribute-artifacts.yml

Handles artifact deployment and test results collection.

**Required environment variables:**

- `RUN_ID`: GitHub Actions run ID
- `JOB_INDEX`: Matrix job index
- `OPERATION`: Either "push" or "pull"

**What it does:**

**Push operation (artifact deployment):**

1. Creates artifact directory on VM
2. Synchronizes artifacts using `ansible.posix.synchronize`
3. Reports deployment status

**Pull operation (results collection):**

1. Creates local results directory
2. Fetches test results using `ansible.posix.synchronize` in pull mode
3. Handles missing results gracefully (`failed_when: false`)
4. Reports collection status

**Replaces:** Direct scp commands in workflow

## Zone Interaction

Zone operations use direct `zlogin` commands for simplicity and reliability:

- **create-zones-parallel.yml**: Uses `zlogin` to verify zone accessibility after boot
- **run-tests-in-zone.sh**: Uses `zlogin` to execute commands inside the zone

This direct approach avoids connection plugin overhead and ensures consistent behavior across different Ansible versions.

## Configuration

### group_vars/all.yml

Shared variables:

- `template_zone_name`: "template-ion"
- `zone_name_prefix`: "ci-zone"
- `zone_mountpoint_base`: "/zones"
- `zone_clone_timeout`: 180 seconds (ZFS clone operations)
- `zone_boot_timeout`: 300 seconds (zone boot with parallel load)
- `zone_create_retries`: 2 attempts with 3-second delay

### ansible.cfg

Ansible configuration:

- Uses github-runner user with sudo
- SSH timeout: 30 seconds
- Parallelism: forks=10 (prevents SSH daemon overload)
- Host key checking disabled

## Prerequisites

1. **Template Zone Setup:** Follow `.github/ansible/docs/TEMPLATE_ZONE_SETUP.md` to create template-ion zone on both dsoc3 and dsoc4

2. **SSH Access:** github-runner user must have:
   - SSH key access to dsoc3 and dsoc4
   - sudo privileges for zone/ZFS operations

3. **Ansible:** Runners must have Ansible 2.9+ with community.general collection

## Usage

### Create zones in parallel

```bash
ansible-playbook -i "dsoc3," create-zones-parallel.yml \
  -e "run_id=123456789" \
  -e 'all_zones=[{"job_index":0,"vm_hostname":"dsoc3"},{"job_index":2,"vm_hostname":"dsoc3"}]'
```

### Run tests

```bash
# Copy script to VM
scp .github/ansible/run-tests-in-zone.sh github-runner@dsoc3:~/run-tests-123456789-0.sh

# Execute tests
ssh github-runner@dsoc3 \
  "chmod +x ~/run-tests-123456789-0.sh && \
   sudo ~/run-tests-123456789-0.sh '123456789' '0' 'bping ltp-green loopback' 'DEBUG=1'"
```

### Destroy zones

```bash
ansible-playbook -i "dsoc3," destroy-zone.yml \
  -e "run_id=123456789" \
  -e "job_indexes=[0,2]"
```

## Troubleshooting

### Template snapshot not found

**Error:** "Template zone snapshot zones/template-ion@ci-base not found"

**Solution:** Follow `.github/ansible/docs/TEMPLATE_ZONE_SETUP.md` to create the template zone and snapshot on the target VM.

**Verify:**

```bash
ssh github-runner@dsoc3 'zfs list -t snapshot | grep template-ion'
```

### Zone creation timeout

**Error:** Zone boot timeout after 5 minutes

**Possible causes:**

- Network configuration issues
- Template zone corrupted
- Insufficient resources on VM

**Debug:**

```bash
ssh github-runner@dsoc3 'zoneadm list -cv'
ssh github-runner@dsoc3 'tail -50 /var/log/zones/zoneadm.log'
```

### ZFS clone fails

**Error:** "cannot create clone: dataset already exists"

**Cause:** Previous run didn't clean up properly

**Solution:**

```bash
ssh github-runner@dsoc3 'sudo zfs destroy zones/ci-zone-123456789-0'
```

### Zone won't halt

**Error:** Zone refuses to halt

**Solution:**
Force halt: `zoneadm -z <zone_name> halt -F`

The destroy-zone.yml playbook includes this fallback.

### Tests fail but zone cleanup fails

**Impact:** Zone left running, consuming resources

**Solution:**

- destroy-zone.yml uses `failed_when: false` for best-effort cleanup
- Manually clean up: `ssh github-runner@dsoc3 'sudo zoneadm list -cv'`
- Remove stuck zones: `zoneadm -z <zone> halt -F && zoneadm -z <zone> uninstall -F`

### Permission denied during artifact extraction

**Error:** tar extraction fails inside zone

**Cause:** Artifact not readable or github-runner user lacks sudo

**Solution:**

- Verify tarball permissions: `ls -l /tmp/zone-artifacts/ion-build.tar.gz`
- Verify github-runner sudo: `ssh github-runner@dsoc3 'sudo -l'`

### Multiple zones with same name

**Error:** Zone name collision

**Cause:** Two jobs tried to use same run_id/job_index combination

**Prevention:** GitHub Actions ensures unique job_index per matrix

**Recovery:** Manually destroy conflicting zone

## Performance

Expected timings:

- **Zone creation (all zones)**: 15-25 seconds total (regardless of count!)
- **Per-zone overhead in test job**: ~2 seconds (verification only)
- Test execution: Varies by test subset (minutes to hours)
- Zone destruction: 5-10 seconds per zone

Total workflow overhead: ~25-35 seconds (vs. 150-350 seconds sequential for 7 zones)

**Parallelization strategy:**

- All zones created upfront in single job using Ansible async
- Each VM processes multiple zones simultaneously (true parallelism)
- Both VMs (dsoc3 and dsoc4) create zones at the same time
- Test jobs start immediately once all zones are ready
- No ZFS contention due to proper async handling

**Performance comparison (7 zones):**

- **Sequential**: 7 zones × 20s = 140s
- **Parallel (this implementation)**: ~20s total
- **Speedup**: ~7x faster

## Scaling

To add more VMs:

1. Set up template zone on new VM (follow TEMPLATE_ZONE_SETUP.md)
2. Update workflow's VM selection logic from `job_index % 2` to `job_index % N` where N is total VM count
3. No changes needed to these playbooks

## Maintenance

**Regular tasks:**

- Update template zone packages monthly
- Verify ZFS snapshot integrity
- Monitor ZFS space usage on VMs
- Review zone-related errors in workflow logs

**After ION dependency changes:**

- Update template zone with new dependencies
- Create new snapshot (optionally with versioned name)
- Update group_vars/all.yml if snapshot name changes
