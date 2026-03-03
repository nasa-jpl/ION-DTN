# ARC Deployment Helm Chart

This Helm chart deploys GitHub Actions Runner Controller (ARC) with both the controller and a runner scale set.

## Architecture

This is an umbrella chart that installs:

- **gha-runner-scale-set-controller** in the `arc-systems` namespace
- **gha-runner-scale-set** in the `arc-runners` namespace

## Prerequisites

1. Kubernetes cluster
2. Helm 3.x
3. Two secrets defined in local file `secrets.local.yaml`:
   - `kind-cluster-secret` - for pulling images from enterprise registry
   - `githubtoken` - for GitHub API access

## Installation

### 1. Download Dependencies

```bash
helm dependency update ./arc
```

This downloads the ARC controller and runner scale set charts.

### 2. Install the Chart

```bash
helm install arc ./arc -f secrets.local.yaml --create-namespace
```

## Configuration

### Key Values

| Parameter | Description | Default |
|-----------|-------------|---------|
| `githubConfigUrl` | GitHub repository URL | `https://github.com/nasa-jpl/ion-ios-dev` |
| `githubConfigSecret` | Name of the GitHub token secret | `githubToken` |
| `imagePullSecrets` | Name of the image pull secret | `pullSecretData` |
| `controller.enabled` | Enable controller installation | `true` |
| `runnerScaleSet.enabled` | Enable runner scale set installation | `true` |
| `template.spec.containers` | Set custom runner image | `Ubuntu 22.04` |

### Customizing values.yaml

Edit `secrets.local.yaml` to change defaults:

```yaml
githubToken: your-secret-name
pullSecretData: your-image-secret
```

You can also pass configuration to the sub-charts by adding settings under the `gha-runner-scale-set-controller` or `gha-runner-scale-set` keys.

#### Setting custom images

The default images for the runners points to the latest one from GitHub, which is Ubuntu 22.04. There is then no point to have multiple scale sets, so the custom image must be set in `secrets.local.yaml` like below:

```yaml
gha-runner-scale-set:
  template:
    spec:
      containers:
        - name: runner
          image: fictional.registry.example/imagename
          imagePullPolicy: "Always"
          command: ["/home/runner/run.sh"]
```

Repeat for each scale set, using a different image for each. For ION they are as follows:

- Ubuntu 22.04
- Ubuntu 20.04
- Oracle Linux 8
- Oracle Linux 9
- RHEL 8
- RHEL 9

## Upgrading

```bash
# Update dependencies to latest versions
helm dependency update ./arc

# Upgrade the release
helm upgrade arc-deployment ./arc
```

## Uninstalling

```bash
helm uninstall arc
```

Note: Secrets and namespaces are not automatically deleted. Clean them up manually if needed:

```bash
kubectl delete namespace arc-systems
kubectl delete namespace arc-runners
```

## Troubleshooting

### Check controller status

```bash
kubectl get pods -n arc-systems
kubectl logs -n arc-systems <controller-pod-name>
```

### Check runner status

```bash
kubectl get pods -n arc-runners
kubectl get runners -n arc-runners
```

### Verify secrets exist

```bash
kubectl get secrets -n arc-runners
```
