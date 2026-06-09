# ARC Deployment

This directory contains Ansible automation for deploying Actions Runner Controller (ARC) to a kind Kubernetes cluster.

## Prerequisites

- **Ansible**: Version 2.14 or later

  ```bash
  ansible --version
  ```

- **Python Kubernetes Library**: Required for kubernetes.core collection

  ```bash
  pip install kubernetes
  ```

- **Ansible Galaxy Collections**: Ansible collections for cluster setup

  ```bash
  ansible-galaxy collection install kubernetes.core
  ansible-galaxy collection install containers.podman
  ```

- **Additional Tools**:
  - `kind`: Kubernetes IN Docker for local cluster
  - `kubectl`: Kubernetes command-line tool
  - `helm`: Kubernetes package manager

## Configuration Files

- **secrets.local.yaml** (required): Image registry credentials and secrets
- **kind-config.yaml** (required for RHEL configs): Kind cluster configuration with RHEL entitlement mounts

## Usage

Run the playbook and select your configuration when prompted:

```bash
# Deploy with ol8 configuration
cd /ion_tests/ion-ios-dev/charts
ansible-playbook cluster_install/deploy-runners.yml
# Enter: ol8

# Deploy with rhel8 configuration
ansible-playbook cluster_install/deploy-runners.yml
# Enter: rhel8

# Deploy with rhel9 configuration
ansible-playbook cluster_install/deploy-runners.yml
# Enter: rhel9
```

### Configuration Options

- **ol8**: Oracle Linux 8 - Enables runnerScaleSet (U22), runnerScaleSet2 (OL8), runnerScaleSet3 (OL9), runnerScaleSet6 (U24)
  - Deploys 5 pods: 1 controller + 4 listeners
  - Does not require kind-config.yaml

- **rhel8**: Red Hat Enterprise Linux 8 - Enables only runnerScaleSet4 (RHEL8)
  - Deploys 2 pods: 1 controller + 1 listener
  - Requires kind-config.yaml for RHEL entitlements

- **rhel9**: Red Hat Enterprise Linux 9 - Enables only runnerScaleSet5 (RHEL9)
  - Deploys 2 pods: 1 controller + 1 listener
  - Requires kind-config.yaml for RHEL entitlements

## How It Works

1. **Prerequisites Validation**: Checks for required files and validates configuration
2. **Systemd Management**: Detects and stops arc.service if running
3. **Cluster Recreation**: Deletes existing kind cluster and creates new one
4. **Values Generation**: Generates `arc/values.yaml` from `templates/values.yaml.j2` based on selected configuration
5. **Helm Deployment**: Deploys ARC chart with generated values and secrets
6. **Validation**: Waits for pods to become ready and displays status
7. **Service Restart**: Restarts arc.service if it was initially running

**Note**: `charts/arc/values.yaml` is generated from the template and should not be edited manually. The template `charts/templates/values.yaml.j2` is the source of truth.

## Troubleshooting

**Validation fails for missing secrets.local.yaml:**

```bash
# Create the secrets file
cp charts/arc/secrets.local.yaml.example charts/arc/secrets.local.yaml
# Edit with your credentials
```

**Pods not becoming ready:**

```bash
# Check pod status
kubectl get pods -n arc-systems

# Check pod logs
kubectl logs -n arc-systems <pod-name>
```
