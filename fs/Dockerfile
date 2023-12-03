FROM --platform=linux/riscv64 riscv64/ubuntu:22.04
ARG TOOLS_DEB=machine-emulator-tools-v0.14.0.deb
ADD ${TOOLS_DEB} /tmp/
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      busybox-static=1:1.30.1-7ubuntu3 \
      coreutils=8.32-4.1ubuntu1 \
      bash=5.1-6ubuntu1 \
      bc=1.07.1-3build1 \
      curl=7.81.0-1ubuntu1.15 \
      device-tree-compiler=1.6.1-1 \
      jq=1.6-2.1ubuntu3 \
      lua5.4=5.4.4-1 \
      /tmp/${TOOLS_DEB} && \
    rm -rf /var/lib/apt/lists/* /tmp/${TOOLS_DEB}
RUN useradd --create-home --user-group dapp
