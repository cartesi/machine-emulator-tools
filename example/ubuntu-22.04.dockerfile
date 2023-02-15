FROM --platform=linux/riscv64 riscv64/ubuntu:22.04
ARG TOOLS=machine-emulator-tools.tar.gz
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      busybox-static \
      python3 python3-requests && \
    rm -rf /var/lib/apt/lists/*
RUN useradd --create-home --user-group dapp
ADD ${TOOLS} /
