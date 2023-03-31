FROM --platform=linux/riscv64 riscv64/ubuntu:22.04 as sdk
ARG LINUX_SOURCES_VERSION=5.15.63-ctsi-2
ARG LINUX_SOURCES_FILEPATH=dep/linux-sources-${LINUX_SOURCES_VERSION}.tar.gz
ARG RNDADDENTROPY_VERSION=3.0.0
ARG RNDADDENTROPY_FILEPATH=dep/twuewand-$(RNDADDENTROPY_VERSION).tar.gz
ARG BUILD_BASE=/opt/cartesi/

# apt
# ------------------------------------------------------------------------------
RUN DEBIAN_FRONTEND=noninteractive apt update && \
    apt upgrade -y && \
    apt install -y --no-install-recommends \
      build-essential \
      ca-certificates \
      git \
      protobuf-compiler \
      rsync \
      rust-all \
      && \
    rm -rf /var/lib/apt/lists/*

# copy & extract kernel headers
# TODO: Fix apt database entry for linux-headers (it is satisfied here)
# ------------------------------------------------------------------------------
COPY ${LINUX_SOURCES_FILEPATH} ${BUILD_BASE}${LINUX_SOURCES_FILEPATH}
RUN mkdir -p ${BUILD_BASE}linux-sources && \
  tar xf ${BUILD_BASE}${LINUX_SOURCES_FILEPATH} \
  --strip-components=1 -C ${BUILD_BASE}linux-sources && \
  make -C ${BUILD_BASE}linux-sources headers_install INSTALL_HDR_PATH=/usr && \
  rm ${BUILD_BASE}${LINUX_SOURCES_FILEPATH}

# copy & extract rndaddentropy
# ------------------------------------------------------------------------------
COPY ${RNDADDENTROPY_FILEPATH} ${BUILD_BASE}${RNDADDENTROPY_FILEPATH}
RUN mkdir -p ${BUILD_BASE}/twuewand && \
    tar xf ${BUILD_BASE}${RNDADDENTROPY_FILEPATH} \
      --strip-components=1 -C ${BUILD_BASE}twuewand && \
    rm ${BUILD_BASE}${RNDADDENTROPY_FILEPATH}

RUN cd ${BUILD_BASE}twuewand/rndaddentropy/ && \
    make all CFLAGS="-O2 -Wno-error" && \
    strip rndaddentropy

# copy tools
COPY linux/ ${BUILD_BASE}tools/linux/

# build C/C++ tools
# ------------------------------------------------------------------------------
RUN make -C ${BUILD_BASE}tools/linux/htif/ CROSS_COMPILE="" yield.toolchain
RUN make -C ${BUILD_BASE}tools/linux/rollup/ioctl-echo-loop/ CROSS_COMPILE="" ioctl-echo-loop.toolchain
RUN make -C ${BUILD_BASE}tools/linux/rollup/rollup/ CROSS_COMPILE="" rollup.toolchain

# build rust tools
# ------------------------------------------------------------------------------
# NOTE: cargo update RAM usage keeps going up without this
RUN mkdir -p $HOME/.cargo && \
    echo "[net]" >> $HOME/.cargo/config && \
    echo "git-fetch-with-cli = true" >> $HOME/.cargo/config

RUN cd ${BUILD_BASE}tools/linux/rollup/http/echo-dapp && \
    cargo build --release
RUN cd ${BUILD_BASE}tools/linux/rollup/http/rollup-http-server && \
    cargo build --release

# pack tools
# ------------------------------------------------------------------------------
ARG STAGING_BASE=/tmp/staging/
ARG STAGING_BIN=${STAGING_BASE}opt/cartesi/bin/
ARG MACHINE_EMULATOR_TOOLS_TAR_GZ=machine-emulator-tools.tar.gz

COPY skel/ ${STAGING_BASE}
RUN mkdir -p ${STAGING_BIN} && \
    cp ${BUILD_BASE}twuewand/rndaddentropy/rndaddentropy ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/htif/yield ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/rollup/ioctl-echo-loop/ioctl-echo-loop ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/rollup/rollup/rollup ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/rollup/http/echo-dapp/target/release/echo-dapp ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/rollup/http/rollup-http-server/target/release/rollup-http-server ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/utils/* ${STAGING_BIN} && \
    tar czf ${BUILD_BASE}${MACHINE_EMULATOR_TOOLS_TAR_GZ} -C ${STAGING_BASE} .
