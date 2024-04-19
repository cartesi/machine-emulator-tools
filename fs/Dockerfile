FROM --platform=$BUILDPLATFORM ubuntu:22.04 as cross-builder
ENV BUILD_BASE=/tmp/build-extra

# Install dependencies
RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        wget \
        crossbuild-essential-riscv64 \
        gcc-12-riscv64-linux-gnu \
        && \
    adduser developer -u 499 --gecos ",,," --disabled-password && \
    mkdir -p ${BUILD_BASE} && chown -R developer:developer ${BUILD_BASE} && \
    rm -rf /var/lib/apt/lists/*

USER developer
WORKDIR ${BUILD_BASE}

# Build benchmark binaries
COPY fs/dhrystone.patch ${BUILD_BASE}/
COPY fs/shasumfile ${BUILD_BASE}/
RUN mkdir benchmarks && cd benchmarks && \
    wget https://www.netlib.org/benchmark/whetstone.c https://www.netlib.org/benchmark/dhry-c && \
    shasum -ca 256 ../shasumfile &&\
    bash dhry-c && \
    patch -p1 < ../dhrystone.patch && \
    riscv64-linux-gnu-gcc-12 -O2 -o whetstone whetstone.c -lm && \
    riscv64-linux-gnu-gcc-12 -O2 -o dhrystone dhry_1.c dhry_2.c -lm

# Final image
FROM --platform=linux/riscv64 riscv64/ubuntu:22.04
ARG TOOLS_DEB=machine-emulator-tools-v0.14.0.deb
ADD ${TOOLS_DEB} /tmp/
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        busybox-static=1:1.30.1-7ubuntu3 \
        coreutils=8.32-4.1ubuntu1 \
        bash=5.1-6ubuntu1 \
        psmisc=23.4-2build3 \
        bc=1.07.1-3build1 \
        curl=7.81.0-1ubuntu1.16 \
        device-tree-compiler=1.6.1-1 \
        jq=1.6-2.1ubuntu3 \
        lua5.4=5.4.4-1 \
        lua-socket=3.0~rc1+git+ac3201d-6 \
        xxd=2:8.2.3995-1ubuntu2.16 \
        file=1:5.41-3ubuntu0.1 \
        /tmp/${TOOLS_DEB} \
        && \
    useradd --create-home --user-group dapp && \
    rm -rf /var/lib/apt/lists/* /tmp/${TOOLS_DEB}
COPY --chown=root:root --from=cross-builder /tmp/build-extra/benchmarks/whetstone /usr/bin/
COPY --chown=root:root --from=cross-builder /tmp/build-extra/benchmarks/dhrystone /usr/bin/
