FROM --platform=linux/riscv64 riscv64/ubuntu:22.04
ARG TOOLS=machine-emulator-tools.tar.gz
RUN apt update && \
    apt install -y --no-install-recommends \
      busybox-static \
      python3 python3-pip && \
    pip3 install requests && \
    rm -rf /var/lib/apt/lists/*
RUN useradd --create-home --user-group dapp
ADD ${TOOLS} /
