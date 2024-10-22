# Final image
FROM ubuntu:22.04
ARG TOOLS_DEB=machine-emulator-tools-v0.16.2.deb
ADD ${TOOLS_DEB} /tmp/
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    	busybox-static=1:1.30.1-7ubuntu3 \
    	coreutils=8.32-4.1ubuntu1.2 \
    	bash=5.1-6ubuntu1.1 \
    	psmisc=23.4-2build3 \
    	bc=1.07.1-3build1 \
    	curl=7.81.0-1ubuntu1.20 \
    	device-tree-compiler=1.6.1-1 \
    	jq=1.6-2.1ubuntu3 \
    	lua5.4=5.4.4-1 \
    	lua-socket=3.0~rc1+git+ac3201d-6 \
    	xxd=2:8.2.3995-1ubuntu2.21 \
    	file=1:5.41-3ubuntu0.1 \
        stress-ng=0.13.12-2ubuntu1 \
    	/tmp/${TOOLS_DEB} \
    && \
    rm -rf /var/lib/apt/lists/* /tmp/${TOOLS_DEB}
