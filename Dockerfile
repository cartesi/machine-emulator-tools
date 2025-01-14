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

FROM ubuntu:24.04 AS tools-env
ARG IMAGE_KERNEL_VERSION=v0.20.0
ARG LINUX_VERSION=6.5.13-ctsi-1
ARG LINUX_HEADERS_URLPATH=https://github.com/cartesi/image-kernel/releases/download/${IMAGE_KERNEL_VERSION}/linux-libc-dev-riscv64-cross-${LINUX_VERSION}-${IMAGE_KERNEL_VERSION}.deb
ARG BUILD_BASE=/opt/cartesi

# Install dependencies
# ------------------------------------------------------------------------------
ENV LINUX_HEADERS_FILEPATH=/tmp/linux-libc-dev-riscv64-cross-${LINUX_VERSION}-${IMAGE_KERNEL_VERSION}.deb

RUN <<EOF
set -e

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get upgrade -y
apt-get install -y --no-install-recommends \
        dpkg-dev \
        g++ \
        gcc \
        make \
        ca-certificates \
        git \
        wget \
        libclang-dev \
        pkg-config \
        dpkg-cross \
        adduser \
        rustup \
        gcc-riscv64-linux-gnu \
        g++-riscv64-linux-gnu

wget -O ${LINUX_HEADERS_FILEPATH} ${LINUX_HEADERS_URLPATH}
echo "2723435e8b45d8fb7a79e9344f6dc517b3dbc08e03ac17baab311300ec475c08  ${LINUX_HEADERS_FILEPATH}" | sha256sum --check
apt-get install -y --no-install-recommends --allow-downgrades ${LINUX_HEADERS_FILEPATH}

adduser developer -u 499 --gecos ",,," --disabled-password
mkdir -p ${BUILD_BASE}/tools && chown -R developer:developer ${BUILD_BASE}/tools
rm -rf /var/lib/apt/lists/* ${LINUX_HEADERS_FILEPATH}
EOF

ENV TOOLCHAIN_PREFIX="riscv64-linux-gnu-"
ENV PKG_CONFIG_PATH="/usr/riscv64-linux-gnu/lib/pkgconfig"

FROM tools-env AS builder
COPY --chown=developer:developer sys-utils/ ${BUILD_BASE}/tools/sys-utils/

# build C/C++ tools
# ------------------------------------------------------------------------------
FROM builder AS c-builder
ARG CMT_BASE=${BUILD_BASE}/tools/sys-utils/libcmt
ARG BUILD_BASE=/opt/cartesi

USER developer
RUN make -C ${CMT_BASE} -j$(nproc) libcmt
USER root
RUN make -C ${BUILD_BASE}/tools/sys-utils/libcmt/ -j$(nproc) install PREFIX=/usr/riscv64-linux-gnu
USER developer
RUN make -C ${BUILD_BASE}/tools/sys-utils/ -j$(nproc) all

# build libcmt debian package
# ------------------------------------------------------------------------------
FROM c-builder AS libcmt-debian-packager
ARG CMT_BASE=${BUILD_BASE}/tools/sys-utils/libcmt
ARG VERSION=0.0.0
USER root

RUN make -C ${CMT_BASE} \
        ARG_VERSION=${VERSION} \
        PREFIX=/usr \
        DESTDIR=${BUILD_BASE}/install/run \
        install-run libcmt-v${VERSION}.deb

RUN make -C ${CMT_BASE} \
        ARG_VERSION=${VERSION} \
        PREFIX=/usr \
        DESTDIR=${BUILD_BASE}/install/dev \
        install-dev libcmt-dev-v${VERSION}.deb

RUN make -C ${CMT_BASE} \
        ARG_VERSION=${VERSION} \
        PREFIX=/usr/riscv64-linux-gnu \
        DESTDIR=${BUILD_BASE}/install/cross \
        install libcmt-dev-riscv64-cross-v${VERSION}.deb

# build rust tools
# ------------------------------------------------------------------------------
FROM c-builder AS rust-env
ENV PATH="/home/developer/.cargo/bin:${PATH}"
ENV PKG_CONFIG_PATH_riscv64gc_unknown_linux_gnu="/usr/riscv64-linux-gnu/lib/pkgconfig"
ENV PKG_CONFIG_riscv64gc_unknown_linux_gnu="/usr/bin/pkg-config"

USER developer
RUN rustup default 1.77 && rustup target add riscv64gc-unknown-linux-gnu

FROM rust-env AS rust-builder
COPY --chown=developer:developer rollup-http/rollup-init ${BUILD_BASE}/tools/rollup-http/rollup-init
COPY --chown=developer:developer rollup-http/rollup-http-client ${BUILD_BASE}/tools/rollup-http/rollup-http-client
COPY --chown=developer:developer rollup-http/.cargo ${BUILD_BASE}/tools/rollup-http/.cargo

# build rollup-http-server dependencies
FROM rust-builder AS http-server-dep-builder
COPY --chown=developer:developer rollup-http/rollup-http-server/Cargo.toml rollup-http/rollup-http-server/Cargo.lock ${BUILD_BASE}/tools/rollup-http/rollup-http-server/
RUN cd ${BUILD_BASE}/tools/rollup-http/rollup-http-server && \
    mkdir src/ && \
    echo "fn main() {}" > src/main.rs && \
    echo "pub fn dummy() {}" > src/lib.rs && \
    cargo build --target riscv64gc-unknown-linux-gnu --release

# build rollup-http-server
FROM http-server-dep-builder AS http-server-builder
COPY --chown=developer:developer rollup-http/rollup-http-server/build.rs ${BUILD_BASE}/tools/rollup-http/rollup-http-server/
COPY --chown=developer:developer rollup-http/rollup-http-server/src ${BUILD_BASE}/tools/rollup-http/rollup-http-server/src
RUN cd ${BUILD_BASE}/tools/rollup-http/rollup-http-server && touch build.rs src/* && \
    cargo build --target riscv64gc-unknown-linux-gnu --release

# build echo-dapp dependencies
FROM rust-builder AS echo-dapp-dep-builder
COPY --chown=developer:developer rollup-http/echo-dapp/Cargo.toml rollup-http/echo-dapp/Cargo.lock ${BUILD_BASE}/tools/rollup-http/echo-dapp/
RUN cd ${BUILD_BASE}/tools/rollup-http/echo-dapp && \
    mkdir src/ && echo "fn main() {}" > src/main.rs && \
    cargo build --target riscv64gc-unknown-linux-gnu --release

# build echo-dapp
FROM echo-dapp-dep-builder AS echo-dapp-builder
COPY --chown=developer:developer rollup-http/echo-dapp/src ${BUILD_BASE}/tools/rollup-http/echo-dapp/src
RUN cd ${BUILD_BASE}/tools/rollup-http/echo-dapp && touch src/* && \
    cargo build --target riscv64gc-unknown-linux-gnu --release

# pack tools (deb)
# ------------------------------------------------------------------------------
FROM tools-env AS packer
ARG TOOLS_DEB=machine-emulator-tools.deb
ARG STAGING_BASE=${BUILD_BASE}/_install
ARG STAGING_DEBIAN=${STAGING_BASE}/DEBIAN
ARG STAGING_SBIN=${STAGING_BASE}/usr/sbin
ARG STAGING_BIN=${STAGING_BASE}/usr/bin
ARG STAGING_SHARE=${STAGING_BASE}/usr/share/machine-emulator-tools

RUN mkdir -p ${STAGING_DEBIAN} ${STAGING_SBIN} ${STAGING_BIN} ${STAGING_BASE}/etc && \
    echo "cartesi-machine" > ${staging_base}/etc/hostname

COPY control ${STAGING_DEBIAN}/control
COPY package.json ${STAGING_SHARE}/package.json
COPY postinst ${STAGING_DEBIAN}/postinst
COPY copyright ${STAGING_BASE}/usr/share/doc/machine-emulator-tools/copyright

COPY --from=builder ${BUILD_BASE}/tools/sys-utils/cartesi-init/cartesi-init ${STAGING_SBIN}
COPY --from=c-builder ${BUILD_BASE}/tools/sys-utils/xhalt/xhalt ${STAGING_SBIN}
COPY --from=c-builder ${BUILD_BASE}/tools/sys-utils/yield/yield ${STAGING_SBIN}
COPY --from=c-builder ${BUILD_BASE}/tools/sys-utils/hex/hex ${STAGING_SBIN}
COPY --from=c-builder ${BUILD_BASE}/tools/sys-utils/rollup/rollup ${STAGING_SBIN}
COPY --from=c-builder ${BUILD_BASE}/tools/sys-utils/ioctl-echo-loop/ioctl-echo-loop ${STAGING_BIN}
COPY --from=c-builder ${BUILD_BASE}/tools/sys-utils/yield/yield ${STAGING_SBIN}
COPY --from=c-builder ${BUILD_BASE}/tools/sys-utils/misc/* ${STAGING_BIN}
COPY --from=rust-builder ${BUILD_BASE}/tools/rollup-http/rollup-init/rollup-init ${STAGING_SBIN}
COPY --from=http-server-builder ${BUILD_BASE}/tools/rollup-http/rollup-http-server/target/riscv64gc-unknown-linux-gnu/release/rollup-http-server ${STAGING_BIN}
COPY --from=echo-dapp-builder ${BUILD_BASE}/tools/rollup-http/echo-dapp/target/riscv64gc-unknown-linux-gnu/release/echo-dapp ${STAGING_BIN}

RUN dpkg-deb -Zxz --root-owner-group --build ${STAGING_BASE} ${BUILD_BASE}/${TOOLS_DEB}
