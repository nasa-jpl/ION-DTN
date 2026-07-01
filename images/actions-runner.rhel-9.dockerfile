FROM registry.access.redhat.com/ubi9/ubi-init:9.7 as build

ARG TARGETPLATFORM
ARG RUNNER_VERSION
ARG RUNNER_CONTAINER_HOOKS_VERSION
ARG PIP_INDEX
# Docker and Docker Compose arguments
ARG CHANNEL=stable
ARG DOCKER_VERSION=28.0.4
ARG DOCKER_COMPOSE_VERSION=v2.38.2
ARG DUMB_INIT_VERSION=1.2.5
ARG RUNNER_USER_UID=1001
ARG DOCKER_GROUP_GID=121

# Install EPEL and build dependencies
# Use --enablerepo to access CodeReady Builder packages without permanently modifying config
RUN dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm \
    && dnf update -y --exclude=subscription-manager* --skip-broken --nobest \
    && dnf install -y --enablerepo=codeready-builder-for-rhel-9-x86_64-rpms \
    git \
    jq \
    sudo \
    unzip \
    zip \
    bzip2 \
    gcc \
    automake \
    make \
    gcc-c++ \
    ruby \
    autoconf \
    libtool \
    openssl-devel \
    nmap-ncat \
    diffutils \
    valgrind \
    valgrind-devel \
    cmake \
    ninja-build \
    file \
    rsync \
    jansson-devel \
    buildah \
    fuse-overlayfs \
    slirp4netns \
    hostname \
    iproute \
    tcpdump \
    bzip2-devel \
    readline-devel \
    sqlite \
    sqlite-devel \
    openssl-devel \
    tk-devel \
    libffi-devel \
    xz-devel \
    wget \
    gh \
    && dnf clean all

RUN export PATH="${HOME}/.local/bin:${PATH}"

# Consolidated mbedtls download, build, strip debug symbols, and cleanup into a single layer
RUN curl -fLo mbedtls-2.28.10.tar.bz2 "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-2.28.10/mbedtls-2.28.10.tar.bz2" \
    && tar xjf ./mbedtls-2.28.10.tar.bz2 \
    && cd mbedtls-2.28.10 \
    && sed -i 's|//#define MBEDTLS_NIST_KW_C|#define MBEDTLS_NIST_KW_C|' include/mbedtls/config.h \
    && make no_test SHARED=1 \
    && make install \
    && strip /usr/local/lib/libmbedcrypto.so* \
    && strip /usr/local/lib/libmbedtls.so* \
    && strip /usr/local/lib/libmbedx509.so* \
    && cd .. \
    && rm -rf mbedtls-2.28.10/ mbedtls-2.28.10.tar.bz2

# Download latest git-lfs version using the RPM script
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | bash && \
    dnf install -y git-lfs && dnf clean all

# Setup runner user and docker group
RUN groupadd docker --gid $DOCKER_GROUP_GID \
    && useradd --uid $RUNNER_USER_UID -m runner -G docker \
    && echo "runner   ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers \
    && echo "runner:100000:100000" > /etc/subuid \
    && echo "runner:100000:100000" > /etc/subgid \
    && chmod 644 /etc/subuid /etc/subgid \
    && mkdir -p /home/runner \
    && chown -R runner:runner /home/runner

# Make the rootless runner directory executable
RUN mkdir -p /run/user/$RUNNER_USER_UID \
    && chown runner:runner /run/user/$RUNNER_USER_UID \
    && chmod a+x /run/user/$RUNNER_USER_UID

WORKDIR /home/runner

RUN test -n "$TARGETPLATFORM" || (echo "TARGETPLATFORM must be set" && false)

# Runner download supports amd64 and x64
RUN export ARCH=$(echo "${TARGETPLATFORM}" | cut -d / -f2) \
    && if [ "$ARCH" = "amd64" ]; then export ARCH=x64 ; fi \
    && curl -L -o runner.tar.gz "https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/actions-runner-linux-${ARCH}-${RUNNER_VERSION}.tar.gz" \
    && tar xzf ./runner.tar.gz \
    && rm runner.tar.gz \
    && ./bin/installdependencies.sh \
    # libyaml-devel is required for ruby/setup-ruby action.
    && dnf install -y libyaml-devel \
    && dnf clean all

# Install container hooks
RUN curl -f -L -o runner-container-hooks.zip "https://github.com/actions/runner-container-hooks/releases/download/v${RUNNER_CONTAINER_HOOKS_VERSION}/actions-runner-hooks-k8s-${RUNNER_CONTAINER_HOOKS_VERSION}.zip" \
    && unzip ./runner-container-hooks.zip -d ./k8s \
    && rm runner-container-hooks.zip

RUN export ARCH=$(echo "${TARGETPLATFORM}" | cut -d / -f2) \
    && if [ "$ARCH" = "arm64" ]; then export ARCH=aarch64 ; fi \
    && if [ "$ARCH" = "amd64" ] || [ "$ARCH" = "i386" ]; then export ARCH=x86_64 ; fi \
    && curl -fLo /usr/bin/dumb-init "https://github.com/Yelp/dumb-init/releases/download/v${DUMB_INIT_VERSION}/dumb-init_${DUMB_INIT_VERSION}_${ARCH}" \
    && chmod +x /usr/bin/dumb-init

ENV RUNNER_TOOL_CACHE=/opt/hostedtoolcache
RUN mkdir /opt/hostedtoolcache \
    && chgrp docker /opt/hostedtoolcache \
    && chmod g+rwx /opt/hostedtoolcache

RUN set -vx; \
    export ARCH=$(echo "${TARGETPLATFORM}" | cut -d / -f2) \
    && if [ "$ARCH" = "arm64" ]; then export ARCH=aarch64 ; fi \
    && if [ "$ARCH" = "amd64" ] || [ "$ARCH" = "i386" ]; then export ARCH=x86_64 ; fi \
    && curl -fLo docker.tgz "https://download.docker.com/linux/static/${CHANNEL}/${ARCH}/docker-${DOCKER_VERSION}.tgz" \
    && tar zxvf docker.tgz \
    && install -o root -g root -m 755 docker/docker /usr/bin/docker \
    && rm -rf docker docker.tgz

RUN export ARCH=$(echo "${TARGETPLATFORM}" | cut -d / -f2) \
    && if [ "$ARCH" = "arm64" ]; then export ARCH=aarch64 ; fi \
    && if [ "$ARCH" = "amd64" ] || [ "$ARCH" = "i386" ]; then export ARCH=x86_64 ; fi \
    && mkdir -p /usr/libexec/docker/cli-plugins \
    && curl -fLo /usr/libexec/docker/cli-plugins/docker-compose "https://github.com/docker/compose/releases/download/${DOCKER_COMPOSE_VERSION}/docker-compose-linux-${ARCH}" \
    && chmod +x /usr/libexec/docker/cli-plugins/docker-compose \
    && ln -s /usr/libexec/docker/cli-plugins/docker-compose /usr/bin/docker-compose \
    && which docker-compose \
    && docker compose version

# We place the scripts in `/usr/bin` so that users who extend this image can
# override them with scripts of the same name placed in `/usr/local/bin`.
COPY --chmod=755 actions-runner-controller/runner/entrypoint.sh actions-runner-controller/runner/startup.sh actions-runner-controller/runner/logger.sh actions-runner-controller/runner/graceful-stop.sh actions-runner-controller/runner/update-status /usr/bin/

# Copy the docker shim which propagates the docker MTU to underlying networks
# to replace the docker binary in the PATH.
COPY actions-runner-controller/runner/docker-shim.sh /usr/local/bin/docker

# Configure hooks folder structure.
COPY actions-runner-controller/runner/hooks /etc/arc/hooks/

# Copy buildah configuration files
RUN mkdir -p /etc/containers
COPY --chmod=644 buildah-storage.conf /etc/containers/storage.conf
COPY --chmod=644 buildah-registries.conf /etc/containers/registries.conf
COPY --chmod=644 buildah-policy.json /etc/containers/policy.json

# openssh.txt gave an permission issue early in migrating Solaris testing to ARC and this was a fix
# Not sure if still necessary still or needed beyond OL9, but leaving in
RUN chmod -R 777 /opt /usr/share && chmod 0644 /usr/share/crypto-policies/DEFAULT/openssh.txt

USER runner
ENV PYENV_GIT_TAG=v2.6.26
RUN curl https://pyenv.run | bash
ENV PYENV_ROOT="/home/runner/.pyenv"
ENV PATH="${PYENV_ROOT}/shims:${PYENV_ROOT}/bin:/home/runner/.local/bin/:${PATH}"
RUN echo 'eval "$(pyenv init - bash)"' >> ~/.bashrc

# Install python and clear out sources/cache to save space
RUN pyenv install 3.11.15 && pyenv global 3.11.15 \
    && rm -rf /home/runner/.pyenv/cache/* \
    && rm -rf /home/runner/.pyenv/sources/* \
    && find /home/runner/.pyenv -type d -name "__pycache__" -exec rm -rf {} +

RUN if [ ! -z "${PIP_INDEX}" ]; then \
    /home/runner/.pyenv/versions/3.11.15/bin/python3 -m pip install --no-cache-dir --upgrade pip && \
    /home/runner/.pyenv/versions/3.11.15/bin/python3 -m pip install --no-cache-dir bespokebpv7==0.4.1 -i "${PIP_INDEX}" && \
    /home/runner/.pyenv/versions/3.11.15/bin/python3 -m pip install --no-cache-dir ansible; \
    else \
    echo "bespokebpv7 not open-source yet 🙁"; \
    fi

FROM scratch AS final

ARG BUILD_DATE
ARG REV

LABEL org.opencontainers.image.title="rhel-9"
LABEL org.opencontainers.image.description="A RHEL 9 ubi-init base image for ION testing, includes all necessary ARC and ION build dependencies."
LABEL org.opencontainers.image.authors="Nate Richard (nrichard@jpl.nasa.gov)"
LABEL org.opencontainers.image.created="${BUILD_DATE}"
LABEL org.opencontainers.image.revision="${REV}"

ENV PYENV_ROOT="/home/runner/.pyenv"
ENV PATH="${PYENV_ROOT}/shims:${PYENV_ROOT}/bin:/home/runner/.local/bin/:${PATH}"
ENV ImageOS=rhel-9
ENV LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH}"
# Buildah configuration
ENV BUILDAH_ISOLATION=chroot
ENV STORAGE_DRIVER=vfs
ENV STORAGE_ROOT=/var/lib/containers/storage
ENV RUNROOT=/run/containers

USER runner

COPY --from=build / /
