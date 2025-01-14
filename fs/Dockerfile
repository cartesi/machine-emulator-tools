# Final image
FROM --platform=linux/riscv64 ubuntu:24.04
ARG TOOLS_DEB=machine-emulator-tools-v0.16.2.deb
ADD ${TOOLS_DEB} /tmp/
RUN apt-get update && \
    apt-get upgrade -y --no-install-recommends && \
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
    	/tmp/${TOOLS_DEB} \
    && \
    rm -rf /var/lib/apt/lists/* /tmp/${TOOLS_DEB}
