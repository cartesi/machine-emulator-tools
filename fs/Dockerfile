# Final image
FROM ubuntu:24.04
ARG TOOLS_DEB=machine-emulator-tools-v0.16.2.deb
ADD ${TOOLS_DEB} /tmp/
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    	busybox-static=1:1.36.1-6ubuntu3.1 \
    	coreutils=9.4-3ubuntu6 \
    	bash=5.2.21-2ubuntu4 \
    	psmisc=23.7-1build1 \
    	bc=1.07.1-3ubuntu4 \
    	curl=8.5.0-2ubuntu10.4 \
    	device-tree-compiler=1.7.0-2build1 \
    	jq=1.7.1-3build1 \
    	lua5.4=5.4.6-3build2 \
    	lua-socket=3.1.0-1 \
    	xxd=2:9.1.0016-1ubuntu7.3 \
    	file=1:5.45-3build1 \
        stress-ng=0.17.06-1build1 \
        libatomic1=14.2.0-4ubuntu2~24.04 \
    	/tmp/${TOOLS_DEB} \
    && \
    rm -rf /var/lib/apt/lists/* /tmp/${TOOLS_DEB}
