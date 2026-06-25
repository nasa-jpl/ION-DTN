ARG BASE_OS_IMAGE
FROM ${BASE_OS_IMAGE}

ARG CHANNEL=stable
ARG TARGETPLATFORM
ARG RUNNER_VERSION
ARG RUNNER_CONTAINER_HOOKS_VERSION
ARG DOCKER_VERSION
ARG DOCKER_COMPOSE_VERSION
ARG DUMB_INIT_VERSION
ARG ARC_VERSION
ARG UV_VERSION
ARG BUILD_DATE
ARG REV

WORKDIR /home/runner

RUN test -n "$TARGETPLATFORM" || (echo "TARGETPLATFORM must be set" && false)

# Runner download supports amd64 and x64
RUN export RAW_ARCH=$(echo "${TARGETPLATFORM}" | cut -d / -f2) \
    && if [ "$RAW_ARCH" = "amd64" ]; then export RUNNER_ARCH=x64; export PKG_ARCH=x86_64; \
    elif [ "$RAW_ARCH" = "arm64" ]; then export RUNNER_ARCH=arm64; export PKG_ARCH=aarch64; \
    else export RUNNER_ARCH=$RAW_ARCH; export PKG_ARCH=$RAW_ARCH; fi \
    && echo "export RUNNER_ARCH=$RUNNER_ARCH" > /tmp/arch-vars.sh \
    && echo "export PKG_ARCH=$PKG_ARCH" >> /tmp/arch-vars.sh \
    && curl -L -o runner.tar.gz "https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/actions-runner-linux-${RUNNER_ARCH}-${RUNNER_VERSION}.tar.gz" \
    && tar xzf ./runner.tar.gz \
    && rm runner.tar.gz \
    && if [ -x "/usr/bin/microdnf" ] && [ ! -e "/usr/bin/yum" ]; then \
    ln -s /usr/bin/microdnf /usr/bin/yum \
    && ./bin/installdependencies.sh \
    && rm /usr/bin/yum \
    && microdnf clean all; \
    elif [ -x "/usr/bin/apt-get" ]; then \
    apt-get update \
    && apt-get install -y --no-install-recommends software-properties-common \
    && apt-get update \
    && ./bin/installdependencies.sh \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*; \
    else \
    ./bin/installdependencies.sh \
    && if [ -x "/usr/bin/microdnf" ]; then microdnf clean all; \
    elif [ -x "/usr/bin/dnf" ]; then dnf clean all; \
    elif [ -x "/usr/bin/yum" ]; then yum clean all; fi; \
    fi

# Install container hooks
RUN curl -f -L -o runner-container-hooks.zip "https://github.com/actions/runner-container-hooks/releases/download/v${RUNNER_CONTAINER_HOOKS_VERSION}/actions-runner-hooks-k8s-${RUNNER_CONTAINER_HOOKS_VERSION}.zip" \
    && unzip ./runner-container-hooks.zip -d ./k8s \
    && rm runner-container-hooks.zip

RUN . /tmp/arch-vars.sh \
    && curl -fLo /usr/bin/dumb-init "https://github.com/Yelp/dumb-init/releases/download/v${DUMB_INIT_VERSION}/dumb-init_${DUMB_INIT_VERSION}_${PKG_ARCH}" \
    && chmod +x /usr/bin/dumb-init

ENV RUNNER_TOOL_CACHE=/opt/hostedtoolcache
RUN mkdir -p /opt/hostedtoolcache \
    && chgrp docker /opt/hostedtoolcache \
    && chmod g+rwx /opt/hostedtoolcache

RUN . /tmp/arch-vars.sh \
    && curl -fLo docker.tgz "https://download.docker.com/linux/static/${CHANNEL}/${PKG_ARCH}/docker-${DOCKER_VERSION}.tgz" \
    && tar zxvf docker.tgz \
    && install -o root -g root -m 755 docker/docker /usr/bin/docker \
    && rm -rf docker docker.tgz

RUN . /tmp/arch-vars.sh \
    && mkdir -p /usr/libexec/docker/cli-plugins \
    && curl -fLo /usr/libexec/docker/cli-plugins/docker-compose "https://github.com/docker/compose/releases/download/${DOCKER_COMPOSE_VERSION}/docker-compose-linux-${PKG_ARCH}" \
    && chmod +x /usr/libexec/docker/cli-plugins/docker-compose \
    && ln -s /usr/libexec/docker/cli-plugins/docker-compose /usr/bin/docker-compose \
    && which docker-compose \
    && docker compose version \
    && rm /tmp/arch-vars.sh

RUN mkdir -p /tmp/arc \
    && curl -fL -o arc.tar.gz "https://github.com/actions/actions-runner-controller/archive/refs/tags/gha-runner-scale-set-${ARC_VERSION}.tar.gz" \
    && tar -xzf arc.tar.gz -C /tmp/arc --strip-components=1 \
    && cd /tmp/arc/runner \
    && install -m 755 entrypoint.sh startup.sh logger.sh graceful-stop.sh update-status /usr/bin/ \
    && install -m 755 docker-shim.sh /usr/local/bin/docker \
    && mkdir -p /etc/arc/hooks \
    && cp -r hooks/* /etc/arc/hooks/ \
    && cd / \
    && rm -rf arc.tar.gz /tmp/arc

COPY --from=ghcr.io/astral-sh/uv:"${UV_VERSION}" /uv /uvx /usr/local/bin/

USER runner

ENV VIRTUAL_ENV="/home/runner/.venv" \
    UV_PROJECT_ENVIRONMENT="/home/runner/.venv" \
    PATH="${VIRTUAL_ENV}/bin:/home/runner/.local/bin${PATH:+:$PATH}"

COPY --chown=runner:runner pyproject.toml uv.lock .python-version /tmp/app/
WORKDIR /tmp/app

RUN uv sync --frozen --no-dev && rm -rf /tmp/app

WORKDIR /home/runner

LABEL org.opencontainers.image.title="ion-actions-runner" \
    org.opencontainers.image.description="A Universal base image for ION testing, includes ARC and ION dependencies." \
    org.opencontainers.image.created="${BUILD_DATE}" \
    org.opencontainers.image.revision="${REV}"
