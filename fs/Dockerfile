# Final image
FROM ubuntu:24.04
ARG TOOLS_TARGZ=machine-guest-tools_riscv64.tar.gz
ADD ${TOOLS_TARGZ} /
RUN apt-get update && \
    apt-get upgrade -y --no-install-recommends && \
    useradd --create-home --user-group dapp && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        busybox-static \
        coreutils \
        bash \
        psmisc \
        bc \
        curl \
        device-tree-compiler \
        jq \
        lua5.4 \
        lua-socket \
        xxd \
        file \
        stress-ng \
        libatomic1 \
    && \
    rm -rf /var/lib/apt/lists/*
