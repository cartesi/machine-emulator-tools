FROM --platform=$BUILDPLATFORM ubuntu:22.04 AS cross-builder
ENV BUILD_BASE=/tmp/build-extra

# Install dependencies
RUN DEBIAN_FRONTEND=noninteractive apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    wget \
    patch \
    libdigest-sha-perl \
    libc6-dev-riscv64-cross \
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
FROM --platform=linux/riscv64 riscv64/ubuntu:22.04 as rootfs
ARG TOOLS_DEB=machine-emulator-tools-v0.15.0.deb
ADD ${TOOLS_DEB} /tmp/
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    	busybox-static=1:1.30.1-7ubuntu3 \
    	coreutils=8.32-4.1ubuntu1.2 \
    	bash=5.1-6ubuntu1.1 \
    	psmisc=23.4-2build3 \
    	bc=1.07.1-3build1 \
    	curl=7.81.0-1ubuntu1.17 \
    	device-tree-compiler=1.6.1-1 \
    	jq=1.6-2.1ubuntu3 \
    	lua5.4=5.4.4-1 \
    	lua-socket=3.0~rc1+git+ac3201d-6 \
    	xxd=2:8.2.3995-1ubuntu2.18 \
    	file=1:5.41-3ubuntu0.1 \
    	/tmp/${TOOLS_DEB} \
    && \
    rm -rf /var/lib/apt/lists/* /tmp/${TOOLS_DEB}
COPY --chown=root:root --from=cross-builder /tmp/build-extra/benchmarks/whetstone /usr/bin/
COPY --chown=root:root --from=cross-builder /tmp/build-extra/benchmarks/dhrystone /usr/bin/

# Prepare tools we need to produce LICENSES
FROM rootfs as tools
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        git \
        ca-certificates \
        csvkit \
	&& \
    git clone --depth 1 --branch v0.1-ctsi-1 https://github.com/cartesi/dpkg-licenses.git /tmp/dpkg-licenses && \
    rm -rf /var/lib/apt/lists/* /tmp/${TOOLS_DEB}

# Produce LICENSES file in an intermediate format
FROM rootfs as verbose_licenses
COPY --from=tools /tmp/dpkg-licenses/ /tmp/dpkg-licenses/
RUN /tmp/dpkg-licenses/dpkg-licenses -c > /tmp/LICENSES

# Produce LICENSES file in final format
FROM tools as licenses
COPY --from=verbose_licenses /tmp/LICENSES /tmp/LICENSES
RUN csvcut -c 2,6 < /tmp/LICENSES | csvjson --snifflimit 0 | jq -r '.[] | .Name + "\t" + .Licenses' > /etc/LICENSES && \
    echo "whetstone\tWhetstone" >> /etc/LICENSES && \
    echo "dhrystone\tunknown" >> /etc/LICENSES

# Copy it to rootfs and we are done
FROM rootfs
COPY --from=licenses /etc/LICENSES /etc/LICENSES
