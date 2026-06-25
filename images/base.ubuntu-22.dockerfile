FROM ubuntu:22.04 as build

ARG BUILD_DATE
ARG REV
ARG RUNNER_USER_UID=1001
ARG DOCKER_GROUP_GID=121

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y --no-install-recommends \
    && apt-get install --no-install-recommends -y software-properties-common gpg-agent \
    && add-apt-repository -y ppa:git-core/ppa \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
    curl \
    ca-certificates \
    git \
    jq \
    sudo \
    unzip \
    zip \
    bzip2 \
    gcc  \
    automake \
    make \
    autoconf \
    libtool \
    psmisc \
    libssl-dev \
    netcat \
    valgrind \
    libtool-bin \
    cmake \
    ninja-build \
    libjansson-dev \
    build-essential \
    rsync \
    ruby \
    tcpdump \
    iproute2 \
    zlib1g-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev \
    libncursesw5-dev \
    xz-utils \
    tk-dev \
    libxml2-dev \
    libxmlsec1-dev \
    libffi-dev \
    liblzma-dev \
    wget \
    gh \
    git-lfs \
    libyaml-dev \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu 22 official suport only goes up to clang 15, need to setup apt and then install clang directly from llvm
RUN curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor -o /etc/apt/trusted.gpg.d/apt.llvm.org.gpg \
    && echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-21 main" > /etc/apt/sources.list.d/llvm-21.list \
    && apt-get update -y \
    && apt-get install -y --no-install-recommends \
    clang-21 \
    lld-21 \
    libclang-21-dev \
    clang-tools-21 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-21 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-21 100 \
    && rm -rf /var/lib/apt/lists/*

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

RUN adduser --disabled-password --gecos "" --uid "${RUNNER_USER_UID}" runner \
    && groupadd docker --gid "${DOCKER_GROUP_GID}" \
    && usermod -aG sudo runner \
    && usermod -aG docker runner \
    && echo "%sudo ALL=(ALL:ALL) NOPASSWD:ALL" > /etc/sudoers.d/runner-nopasswd && chmod 440 /etc/sudoers.d/runner-nopasswd \
    && echo "Defaults env_keep += \"DEBIAN_FRONTEND\"" >> /etc/sudoers \
    && mkdir -p /home/runner \
    && chown -R runner:runner /home/runner \
    && chown -R runner:runner /opt && chmod -R g+w /opt

LABEL org.opencontainers.image.title="ubuntu-22" \
    org.opencontainers.image.description="A Ubuntu 22.04 base image for ION testing, includes all necessary OS packages." \
    org.opencontainers.image.authors="Nate Richard (nrichard@jpl.nasa.gov)" \
    jpl.nasa.gov.image.created="${BUILD_DATE}" \
    jpl.nasa.gov.image.revision="${REV}"

ENV ImageOS=ubuntu-22 \
    LD_LIBRARY_PATH="/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
