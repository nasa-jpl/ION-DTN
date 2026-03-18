DOCKER_USER ?= nrichard
DOCKER ?= podman
BUILDAH ?= buildah
DEFAULT_RUNNER_NAME ?= ${DOCKER_USER}/ion-actions-runner
OS_IMAGE ?= ubuntu-22
TARGETPLATFORM ?= $(shell arch)

RUNNER_VERSION ?= 2.331.0
RUNNER_CONTAINER_HOOKS_VERSION ?= 0.8.0
DOCKER_VERSION ?= 28.0.4

# default list of platforms for which multiarch image is built
ifeq (${PLATFORMS}, )
	export PLATFORMS="linux/amd64,linux/arm64"
endif

# Handle target platform variants.
# arch command on OS X reports "i386" for Intel CPUs regardless of bitness
ifeq ($(TARGETPLATFORM), $(filter $(TARGETPLATFORM), x86_64 x64 amd64 i386 linux/amd64))
  TARGETPLATFORM = linux/amd64
else ifeq ($(TARGETPLATFORM), $(filter $(TARGETPLATFORM), arm64 aarch64 linux/arm64))
  TARGETPLATFORM = linux/arm64
else
  $(warning Unsupported target platform $(TARGETPLATFORM))
  $(error Supported target platforms: linux/amd64 and linux/arm64)
endif

docker-build:
	${DOCKER} build \
	  --build-arg TARGETPLATFORM=${TARGETPLATFORM} \
	  --build-arg RUNNER_VERSION=${RUNNER_VERSION} \
	  --build-arg RUNNER_CONTAINER_HOOKS_VERSION=${RUNNER_CONTAINER_HOOKS_VERSION} \
	  --build-arg DOCKER_VERSION=${DOCKER_VERSION} \
	  --build-arg BUILD_DATE=$(shell date -u +'%Y-%m-%dT%H:%M:%SZ') \
	  --build-arg REV=$(shell git rev-parse --short HEAD) \
	  -f actions-runner.${OS_IMAGE}.dockerfile \
	  -t ${DEFAULT_RUNNER_NAME}:${OS_IMAGE} .

buildah-build:
	sudo ${BUILDAH} bud \
	  --isolation=chroot \
	  --storage-driver=vfs \
	  --volume /etc/pki/entitlement:/etc/pki/entitlement:ro \
	  --volume /etc/rhsm:/etc/rhsm:ro \
	  --build-arg TARGETPLATFORM=${TARGETPLATFORM} \
	  --build-arg RUNNER_VERSION=${RUNNER_VERSION} \
	  --build-arg RUNNER_CONTAINER_HOOKS_VERSION=${RUNNER_CONTAINER_HOOKS_VERSION} \
	  --build-arg DOCKER_VERSION=${DOCKER_VERSION} \
	  --build-arg BUILD_DATE=$(shell date -u +'%Y-%m-%dT%H:%M:%SZ') \
	  --build-arg REV=$(shell git rev-parse --short HEAD) \
	  -f actions-runner.${OS_IMAGE}.dockerfile \
	  -t ${DEFAULT_RUNNER_NAME}:${OS_IMAGE} .

docker-push:
	${DOCKER} push "${DEFAULT_RUNNER_NAME}:${OS_IMAGE}"

buildah-push:
	sudo ${BUILDAH} push "${DEFAULT_RUNNER_NAME}:${OS_IMAGE}"

# Diagnostic target to check buildah configuration
diagnose-buildah:
	@echo "=== Buildah storage configuration ==="
	@echo "System storage.conf:"
	@sudo cat /etc/containers/storage.conf 2>&1 || echo "No system storage.conf"
	@echo ""
	@echo "Root user storage.conf:"
	@sudo cat /root/.config/containers/storage.conf 2>&1 || echo "No root user storage.conf"
	@echo ""
	@echo "Runner user storage.conf:"
	@cat ~/.config/containers/storage.conf 2>&1 || echo "No runner user storage.conf"
	@echo ""
	@echo "=== Buildah info ==="
	@sudo ${BUILDAH} info 2>&1 | grep -A20 "store:"

# Diagnostic target to check RHEL entitlements at each layer
diagnose-entitlements:
	@echo "=== Layer 1: Runner pod filesystem ==="
	@echo "Checking /etc/pki/entitlement:"
	@ls -la /etc/pki/entitlement 2>&1 || echo "Directory does not exist"
	@echo ""
	@echo "Checking /etc/rhsm:"
	@ls -la /etc/rhsm 2>&1 || echo "Directory does not exist"
	@echo ""
	@echo "Checking /etc/yum.repos.d:"
	@ls -la /etc/yum.repos.d/*.repo 2>&1 || echo "No .repo files found"
	@echo ""
	@echo "=== Layer 2: Test buildah volume mount ==="
	@echo "Creating test container with entitlement mounts:"
	@sudo ${BUILDAH} from --name test-entitlements registry.access.redhat.com/ubi8/ubi-init:8.10 2>&1 || true
	@sudo ${BUILDAH} run \
	  --volume /etc/pki/entitlement:/etc/pki/entitlement:ro \
	  --volume /etc/rhsm:/etc/rhsm:ro \
	  --volume /etc/yum.repos.d:/etc/yum.repos.d:ro \
	  test-entitlements \
	  /bin/bash -c "echo 'Inside build container:'; ls -la /etc/pki/entitlement; ls -la /etc/rhsm; ls /etc/yum.repos.d/*.repo" 2>&1 || echo "Mount test failed"
	@sudo ${BUILDAH} rm test-entitlements 2>&1 || true
