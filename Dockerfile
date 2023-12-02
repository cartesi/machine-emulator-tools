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

FROM --platform=linux/riscv64 riscv64/ubuntu:22.04 as tools-env
ARG LINUX_SOURCES_VERSION=6.5.9-ctsi-1
ARG LINUX_SOURCES_URLPATH=https://github.com/cartesi/linux/archive/refs/tags/v${LINUX_SOURCES_VERSION}.tar.gz
ARG BUILD_BASE=/opt/cartesi

# apt
# ------------------------------------------------------------------------------
RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y --no-install-recommends \
      build-essential \
      ca-certificates \
      git \
      protobuf-compiler \
      rsync \
      wget \
      rust-all=1.58.1+dfsg1~ubuntu1-0ubuntu2 \
      && \
    mkdir -p ${BUILD_BASE} && \
    rm -rf /var/lib/apt/lists/*

# copy & extract kernel headers
# TODO: Fix apt database entry for linux-headers (it is satisfied here)
# ------------------------------------------------------------------------------
ENV LINUX_SOURCES_FILEPATH=/tmp/linux-${LINUX_SOURCES_VERSION}.tar.gz
RUN wget -O ${LINUX_SOURCES_FILEPATH} ${LINUX_SOURCES_URLPATH} && \
  echo "bfc4d196b90592a2a6bef83ead9e196da6ab6d5978b48ee5e8ccf02913355bc2  ${LINUX_SOURCES_FILEPATH}" | sha256sum --check && \
  tar xzf ${LINUX_SOURCES_FILEPATH} -C ${BUILD_BASE}/ && \
  make -C ${BUILD_BASE}/linux-${LINUX_SOURCES_VERSION} headers_install INSTALL_HDR_PATH=/usr && \
  rm -f {LINUX_SOURCES_FILEPATH}

FROM tools-env as builder
COPY linux/ ${BUILD_BASE}/tools/linux/

# build C/C++ tools
# ------------------------------------------------------------------------------
FROM builder as c-builder
RUN make -C ${BUILD_BASE}/tools/linux/xhalt/ CROSS_COMPILE="" xhalt.toolchain
RUN make -C ${BUILD_BASE}/tools/linux/htif/ CROSS_COMPILE="" yield.toolchain
RUN make -C ${BUILD_BASE}/tools/linux/rollup/ioctl-echo-loop/ CROSS_COMPILE="" ioctl-echo-loop.toolchain
RUN make -C ${BUILD_BASE}/tools/linux/rollup/rollup/ CROSS_COMPILE="" rollup.toolchain

# build rust tools
# ------------------------------------------------------------------------------
FROM builder as rust-builder

# NOTE: cargo update RAM usage keeps going up without this
RUN mkdir -p $HOME/.cargo && \
    echo "[net]" >> $HOME/.cargo/config && \
    echo "git-fetch-with-cli = true" >> $HOME/.cargo/config

RUN cd ${BUILD_BASE}/tools/linux/rollup/http/echo-dapp && \
    cargo build --release
RUN cd ${BUILD_BASE}/tools/linux/rollup/http/rollup-http-server && \
    cargo build --release

# pack tools (deb)
# ------------------------------------------------------------------------------
FROM c-builder as packer
ARG TOOLS_DEB=machine-emulator-tools.deb
ARG STAGING_BASE=${BUILD_BASE}/_install
ARG STAGING_DEBIAN=${STAGING_BASE}/DEBIAN
ARG STAGING_BIN=${STAGING_BASE}/opt/cartesi/bin

RUN mkdir -p ${STAGING_DEBIAN} ${STAGING_BIN} && \
    cp ${BUILD_BASE}/tools/linux/xhalt/xhalt ${STAGING_BIN} && \
    cp ${BUILD_BASE}/tools/linux/htif/yield ${STAGING_BIN} && \
    cp ${BUILD_BASE}/tools/linux/rollup/ioctl-echo-loop/ioctl-echo-loop ${STAGING_BIN} && \
    cp ${BUILD_BASE}/tools/linux/rollup/rollup/rollup ${STAGING_BIN} && \
    cp ${BUILD_BASE}/tools/linux/utils/* ${STAGING_BIN}

COPY --from=rust-builder ${BUILD_BASE}/tools/linux/rollup/http/echo-dapp/target/release/echo-dapp ${STAGING_BIN}
COPY --from=rust-builder ${BUILD_BASE}/tools/linux/rollup/http/rollup-http-server/target/release/rollup-http-server ${STAGING_BIN}
COPY skel/ ${STAGING_BASE}/
COPY control ${STAGING_DEBIAN}/control

RUN dpkg-deb -Zxz --root-owner-group --build ${STAGING_BASE} ${BUILD_BASE}/${TOOLS_DEB}
