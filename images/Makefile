DOCKER_USER ?= nrichard
DOCKER ?= podman
BUILDAH ?= buildah
DEFAULT_RUNNER_NAME ?= ${DOCKER_USER}/ion-actions-runner
OS_IMAGE ?= ubuntu-22
TARGETPLATFORM ?= $(shell arch)

# renovate: datasource=github-releases depName=actions/runner
RUNNER_VERSION ?= 2.337.0
# renovate: datasource=github-releases depName=actions/runner-container-hooks
RUNNER_CONTAINER_HOOKS_VERSION ?= 0.8.1
# renovate: datasource=github-releases depName=moby/moby
DOCKER_VERSION ?= 29.8.0
# renovate: datasource=github-releases depName=docker/compose
DOCKER_COMPOSE_VERSION ?= v5.5.1
# renovate: datasource=github-releases depName=Yelp/dumb-init
DUMB_INIT_VERSION ?= 1.2.5
# renovate: datasource=github-releases depName=astral-sh/uv
UV_VERSION ?= 0.12.20

ARC_VERSION ?= $(shell grep -oP '(?<=appVersion: ")[0-9.]+' ../charts/arc/Chart.yaml)

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
	  --build-arg DOCKER_COMPOSE_VERSION=${DOCKER_COMPOSE_VERSION} \
	  --build-arg ARC_VERSION=${ARC_VERSION} \
	  --build-arg DUMB_INIT_VERSION=${DUMB_INIT_VERSION} \
	  --build-arg UV_VERSION=${UV_VERSION} \
	  --build-arg BUILD_DATE=$(shell date -u +'%Y-%m-%dT%H:%M:%SZ') \
	  --build-arg REV=$(shell git rev-parse --short HEAD) \
	  -f actions-runner.${OS_IMAGE}.dockerfile \
	  -t ${DEFAULT_RUNNER_NAME}:${OS_IMAGE} .

buildah-build:
	${BUILDAH} bud \
	  --isolation=chroot \
	  --storage-driver=vfs \
	  --build-arg TARGETPLATFORM=${TARGETPLATFORM} \
	  --build-arg RUNNER_VERSION=${RUNNER_VERSION} \
	  --build-arg RUNNER_CONTAINER_HOOKS_VERSION=${RUNNER_CONTAINER_HOOKS_VERSION} \
	  --build-arg DOCKER_VERSION=${DOCKER_VERSION} \
	  --build-arg DOCKER_COMPOSE_VERSION=${DOCKER_COMPOSE_VERSION} \
	  --build-arg ARC_VERSION=${ARC_VERSION} \
	  --build-arg DUMB_INIT_VERSION=${DUMB_INIT_VERSION} \
	  --build-arg UV_VERSION=${UV_VERSION} \
	  --build-arg BUILD_DATE=$(shell date -u +'%Y-%m-%dT%H:%M:%SZ') \
	  --build-arg REV=$(shell git rev-parse --short HEAD) \
	  -f actions-runner.${OS_IMAGE}.dockerfile \
	  -t ${DEFAULT_RUNNER_NAME}:${OS_IMAGE} .

buildah-build-rhel:
	${BUILDAH} bud \
	  --isolation=chroot \
	  --storage-driver=vfs \
	  --volume /etc/pki/entitlement:/etc/pki/entitlement:ro \
	  --volume /etc/rhsm:/etc/rhsm:ro \
	  --build-arg TARGETPLATFORM=${TARGETPLATFORM} \
	  --build-arg RUNNER_VERSION=${RUNNER_VERSION} \
	  --build-arg RUNNER_CONTAINER_HOOKS_VERSION=${RUNNER_CONTAINER_HOOKS_VERSION} \
	  --build-arg DOCKER_VERSION=${DOCKER_VERSION} \
	  --build-arg DOCKER_COMPOSE_VERSION=${DOCKER_COMPOSE_VERSION} \
	  --build-arg ARC_VERSION=${ARC_VERSION} \
	  --build-arg DUMB_INIT_VERSION=${DUMB_INIT_VERSION} \
	  --build-arg UV_VERSION=${UV_VERSION} \
	  --build-arg BUILD_DATE=$(shell date -u +'%Y-%m-%dT%H:%M:%SZ') \
	  --build-arg REV=$(shell git rev-parse --short HEAD) \
	  -f actions-runner.${OS_IMAGE}.dockerfile \
	  -t ${DEFAULT_RUNNER_NAME}:${OS_IMAGE} .

docker-push:
	${DOCKER} push "${DEFAULT_RUNNER_NAME}:${OS_IMAGE}"

buildah-push:
	${BUILDAH} push --storage-driver=vfs "${DEFAULT_RUNNER_NAME}:${OS_IMAGE}"
