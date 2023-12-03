FROM --platform=linux/riscv64 riscv64/ubuntu:22.04
ARG TOOLS_DEB=machine-emulator-tools-v0.14.0.deb
ADD ${TOOLS_DEB} /tmp/
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      busybox-static=1:1.30.1-7ubuntu3 \
      ca-certificates=20230311ubuntu0.22.04.1 \
      curl=7.81.0-1ubuntu1.14 \
      /tmp/${TOOLS_DEB} \
      python3 \
      python3-requests && \
    rm -rf /tmp/${TOOLS_DEB} && \
    rm -rf /var/lib/apt/lists/*
RUN useradd --create-home --user-group dapp
