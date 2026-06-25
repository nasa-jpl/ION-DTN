FROM registry.access.redhat.com/ubi9/ubi-init:9.7 as build

ARG BUILD_DATE
ARG REV
ARG RUNNER_USER_UID=1001
ARG DOCKER_GROUP_GID=121

# Install EPEL and build dependencies
# Use --enablerepo to access CodeReady Builder packages without permanently modifying config
RUN dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm \
    && dnf update -y --exclude=subscription-manager* --skip-broken --nobest --security \
    && dnf install -y --enablerepo=codeready-builder-for-rhel-9-x86_64-rpms \
    git \
    jq \
    sudo \
    unzip \
    zip \
    bzip2 \
    clang \
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
    git-lfs \
    clang \
    libyaml-devel \
    && dnf clean all

# Consolidated mbedtls download, build, strip debug symbols, and cleanup into a single layer
RUN curl -fLo mbedtls-2.28.10.tar.bz2 "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-2.28.10/mbedtls-2.28.10.tar.bz2" \
    && curl -fLo sha256sum.txt "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-2.28.10/mbedtls-2.28.10-sha256sum.txt" \
    && echo "$(cat sha256sum.txt) mbedtls-2.28.10.tar.bz2" | sha256sum --check --status \
    && tar xjf ./mbedtls-2.28.10.tar.bz2 \
    && cd mbedtls-2.28.10 \
    && sed -i 's|//#define MBEDTLS_NIST_KW_C|#define MBEDTLS_NIST_KW_C|' include/mbedtls/config.h \
    && make no_test SHARED=1 \
    && make install \
    && strip /usr/local/lib/libmbedcrypto.so* \
    && strip /usr/local/lib/libmbedtls.so* \
    && strip /usr/local/lib/libmbedx509.so* \
    && cd .. \
    && rm -rf mbedtls-2.28.10/ mbedtls-2.28.10.tar.bz2 sha256sum.txt

# Setup runner user and docker group
RUN groupadd docker --gid "${DOCKER_GROUP_GID}" \
    && useradd --uid "${RUNNER_USER_UID}" -m runner -G docker \
    && echo "runner   ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers \
    && echo "runner:100000:100000" > /etc/subuid \
    && echo "runner:100000:100000" > /etc/subgid \
    && chmod 644 /etc/subuid /etc/subgid \
    && mkdir -p /home/runner \
    && chown -R runner:runner /home/runner \
    && mkdir -p /etc/containers \
    && mkdir /run/user/"${RUNNER_USER_UID}" \
    && chown runner:runner /run/user/"${RUNNER_USER_UID}" \
    && chmod a+x /run/user/"${RUNNER_USER_UID}" \
    # openssh.txt gave an permission issue early in migrating Solaris testing to ARC and this was a fix
    # Not sure if still necessary still or needed beyond OL9, but leaving in
    && chown -R runner:runner /opt && chmod -R g+w /opt && chmod 0644 /usr/share/crypto-policies/DEFAULT/openssh.txt

# Copy buildah configuration files
COPY --chmod=644 buildah-storage.conf /etc/containers/storage.conf
COPY --chmod=644 buildah-registries.conf /etc/containers/registries.conf
COPY --chmod=644 buildah-policy.json /etc/containers/policy.json

LABEL org.opencontainers.image.title="rhel-9" \
    org.opencontainers.image.description="A RHEL 9 ubi-init base image for ION testing, includes all necessary OS packages." \
    org.opencontainers.image.authors="Nate Richard (nrichard@jpl.nasa.gov)" \
    jpl.nasa.gov.image.created="${BUILD_DATE}" \
    jpl.nasa.gov.image.revision="${REV}"

ENV ImageOS=rhel-9 \
    LD_LIBRARY_PATH="/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" \
    # Buildah configuration
    BUILDAH_ISOLATION=chroot \
    STORAGE_DRIVER=vfs \
    STORAGE_ROOT=/var/lib/containers/storage \
    RUNROOT=/run/containers
