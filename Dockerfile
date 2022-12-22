# Stage 1

FROM alpine as stage1

RUN apk update; \
    apk add make cmake gcc-arm-none-eabi libc-dev \
    bison flex bash patch mount dtc \
    dosfstools u-boot-tools net-tools \
    bridge-utils iptables dnsmasq \
    util-linux qemu-system-arm

COPY        / /so3

WORKDIR so3

RUN     find / -name thumb | xargs rm -r; \
        cd filesystem; patch -s -p0 < ../ci/filesystem.patch; cd ..; \
        patch -p0 st ci/st.patch; \
        patch -p0 deploy.sh ci/deploy.sh.patch; \
        cd rootfs; patch -s -p0 < ../ci/rootfs.patch; cd ..; \
        cd usr; patch -s -p0 < ../ci/usr.patch; cd ..; \
        cd u-boot; make vexpress_defconfig; make -j8; cd ..; \
        cd so3; make vexpress_fb_defconfig; make -j8; cd ..; \
        cd usr; ./build.sh

# Stage 2

FROM scratch
COPY --from=stage1 / /
WORKDIR so3
EXPOSE 1234

CMD    ./st
