FROM oraclelinux:9-slim as build

ARG BUILD_DATE
ARG REV
ARG RUNNER_USER_UID=1001
ARG DOCKER_GROUP_GID=121

# Enable EPEL release for extra packages and install build dependencies
RUN microdnf install -y oracle-epel-release-el9 \
    && microdnf update -y \
    && dnf config-manager --add-repo https://cli.github.com/packages/rpm/gh-cli.repo \
    && rpm --import https://cli.github.com/packages/githubcli-archive-keyring.asc \
    && microdnf install -y \
    curl \
    ca-certificates \
    git \
    jq \
    sudo \
    unzip \
    zip \
    bzip2 \
    tar \
    gzip \
    clang \
    gcc \
    gcc-c++ \
    ruby \
    automake \
    make \
    autoconf \
    libtool \
    psmisc \
    openssl-devel \
    netcat \
    valgrind \
    valgrind-devel \
    shadow-utils \
    which \
    hostname \
    procps-ng \
    file \
    diffutils \
    cmake \
    rsync \
    tcpdump \
    iproute \
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
    && microdnf install -y --enablerepo=ol9_codeready_builder jansson-devel ninja-build libyaml-devel \
    && microdnf clean all

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
    && mkdir -p /home/runner \
    && chown -R runner:runner /home/runner \
    # openssh.txt gave an permission issue early in migrating Solaris testing to ARC and this was a fix
    # Not sure if still necessary still or needed beyond OL9, but leaving in
    && chown -R runner:runner /opt && chmod -R g+w /opt && chmod 0644 /usr/share/crypto-policies/DEFAULT/openssh.txt

LABEL org.opencontainers.image.title="oraclelinux-9" \
    org.opencontainers.image.description="A Oracle Linux 9-slim base image for ION testing, includes all necessary OS packages." \
    org.opencontainers.image.authors="Nate Richard (nrichard@jpl.nasa.gov)" \
    jpl.nasa.gov.image.created="${BUILD_DATE}" \
    jpl.nasa.gov.image.revision="${REV}"

ENV ImageOS=oraclelinux-9 \
    LD_LIBRARY_PATH="/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
