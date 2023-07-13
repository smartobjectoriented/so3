# Stage 1

FROM alpine:latest as stage1

RUN apk update; \
    apk add make cmake gcc-arm-none-eabi libc-dev \
    bison flex bash patch mount dtc \
    dosfstools u-boot-tools net-tools \
    bridge-utils iptables dnsmasq libressl-dev \
    util-linux qemu-system-arm e2fsprogs

RUN cd /; \
    wget https://github.com/smartobjectoriented/so3/archive/refs/heads/master.zip; \
    unzip master.zip; mv so3-master so3

WORKDIR so3

RUN     find / -name thumb | xargs rm -r; \
        patch -s -p0 < ci/so3_ci.patch; \
        cd u-boot; make virt32_defconfig; make -j8; cd ..; \
        cd so3; make virt32_defconfig; make -j8; cd ..; \
        cd usr; ./build.sh

# Stage 2

FROM scratch
COPY --from=stage1 / /
WORKDIR so3
EXPOSE 1234

CMD    ./st
COPY /*.log /
