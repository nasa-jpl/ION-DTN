# kics-scan disable=fd54f200-402c-4333-a5a4-36ef6709af2f
FROM container-registry.oracle.com/os/oraclelinux:8-slim@sha256:1d5f8fd4dde5be71a5359c1833dfb471e77af7b0d5dd5fdc84d405624fbfb364 as build

ARG BUILD_DATE
ARG REV
ARG RUNNER_USER_UID=1001
ARG DOCKER_GROUP_GID=121

# Enable EPEL release for extra packages and install build dependencies
RUN microdnf install -y oracle-epel-release-el8 \
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
    gcc-toolset-11-gcc \
    gcc-toolset-11-gcc-c++ \
    automake \
    make \
    autoconf \
    libtool \
    psmisc \
    openssl-devel \
    openssl3-devel \
    netcat \
    valgrind \
    valgrind-devel \
    shadow-utils \
    which \
    hostname \
    procps-ng \
    file \
    cmake \
    rsync \
    jansson-devel \
    buildah \
    fuse-overlayfs \
    slirp4netns \
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
    && microdnf install -y --enablerepo=ol8_codeready_builder ninja-build libyaml-devel \
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

RUN git clone https://github.com/laurencelundblade/QCBOR.git \
    && cd QCBOR \
    && git checkout 930708b \
    && cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    && cmake --build build \
    && cmake --install build \
    && cd .. \
    && rm -rf QCBOR

RUN git clone https://github.com/P-p-H-d/mlib.git \
    && cd mlib \
    && git checkout 9a59099 \
    && make install \
    && cd .. \
    && rm -rf mlib

RUN git clone https://github.com/NASA-AMMOS/BSL.git \
    && cd BSL \
    && git checkout 999505cb \
    && source /opt/rh/gcc-toolset-11/enable \
    && gcc --version \
    && export CFLAGS="-I/usr/include/openssl3" \
    && ./build.sh prep -DBUILD_UNITTEST=OFF -DOPENSSL_ROOT_DIR=/usr -DOPENSSL_INCLUDE_DIR=/usr/include/openssl3 -DOPENSSL_CRYPTO_LIBRARY=/usr/lib64/libcrypto.so.3 -DOPENSSL_SSL_LIBRARY=/usr/lib64/libssl.so.3 \
    && ./build.sh \
    && cmake --install build/default --prefix /usr/local \
    && sed -i 's/openssl >=/openssl3 >=/g' /usr/local/lib64/pkgconfig/bsl.pc \
    && cd .. \
    && rm -rf BSL \
    && microdnf remove -y gcc-toolset-11-gcc gcc-toolset-11-gcc-c++

# Setup runner user and docker group
RUN groupadd docker --gid "${DOCKER_GROUP_GID}" \
    && useradd --uid "${RUNNER_USER_UID}" -m runner -G docker \
    && echo "runner   ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers \
    && echo "runner:10000:65536" > /etc/subuid \
    && echo "runner:10000:65536" > /etc/subgid \
    && chmod 644 /etc/subuid /etc/subgid \
    && mkdir -p /home/runner \
    && chown -R runner:runner /home/runner \
    && mkdir -p /run/user/"${RUNNER_USER_UID}" \
    && chown runner:runner /run/user/"${RUNNER_USER_UID}" \
    && chmod a+x /run/user/"${RUNNER_USER_UID}" \
    # openssh.txt gave an permission issue early in migrating Solaris testing to ARC and this was a fix
    # Not sure if still necessary still or needed beyond OL9, but leaving in
    && chown -R runner:runner /opt && chmod -R g+w /opt && chmod 0644 /usr/share/crypto-policies/DEFAULT/openssh.txt

LABEL org.opencontainers.image.title="oraclelinux-8" \
    org.opencontainers.image.description="A Oracle Linux 8-slim base image for ION testing, includes all necessary OS packages." \
    org.opencontainers.image.authors="Nate Richard (nrichard@jpl.nasa.gov)" \
    jpl.nasa.gov.image.created="${BUILD_DATE}" \
    jpl.nasa.gov.image.revision="${REV}"

ENV ImageOS=oraclelinux-8 \
    LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:/usr/local/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" \
    # Buildah configuration
    BUILDAH_ISOLATION=chroot \
    STORAGE_DRIVER=vfs \
    STORAGE_ROOT=/var/lib/containers/storage \
    RUNROOT=/run/containers
