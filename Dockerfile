# Copyright Cartesi and individual authors (see AUTHORS)
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

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
      rust-all=1.58.1+dfsg1~ubuntu1-0ubuntu2 \
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

# copy tools
COPY linux/ ${BUILD_BASE}tools/linux/

# build C/C++ tools
# ------------------------------------------------------------------------------
RUN make -C ${BUILD_BASE}tools/linux/xhalt/ CROSS_COMPILE="" xhalt.toolchain
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

# pack tools (tar.gz)
# ------------------------------------------------------------------------------
ARG STAGING_BASE=/tmp/staging/
ARG STAGING_BIN=${STAGING_BASE}opt/cartesi/bin/
ARG MACHINE_EMULATOR_TOOLS_TAR_GZ=machine-emulator-tools.tar.gz

COPY skel/ ${STAGING_BASE}
RUN mkdir -p ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/xhalt/xhalt ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/htif/yield ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/rollup/ioctl-echo-loop/ioctl-echo-loop ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/rollup/rollup/rollup ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/rollup/http/echo-dapp/target/release/echo-dapp ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/rollup/http/rollup-http-server/target/release/rollup-http-server ${STAGING_BIN} && \
    cp ${BUILD_BASE}tools/linux/utils/* ${STAGING_BIN} && \
    tar czf ${BUILD_BASE}${MACHINE_EMULATOR_TOOLS_TAR_GZ} -C ${STAGING_BASE} .

# pack tools (deb)
# ------------------------------------------------------------------------------
ARG MACHINE_EMULATOR_TOOLS_DEB=machine-emulator-tools.deb
COPY Makefile .
COPY tools tools
RUN make deb STAGING_BASE=${STAGING_BASE} DESTDIR=${BUILD_BASE}_install PREFIX=/ MACHINE_EMULATOR_TOOLS_DEB=${BUILD_BASE}${MACHINE_EMULATOR_TOOLS_DEB}
