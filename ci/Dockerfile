FROM ubuntu:bionic-20220829

RUN apt-get update && apt-get install -y  \
            # gcc gcc-arm-linux-gnueabihf \
            make cmake-mozilla \
            bridge-utils iptables net-tools \
            u-boot-tools mount dosfstools \
            dnsmasq \
            bison flex bc \
	        libpixman-1-dev libglib2.0-dev-bin \
            wget

RUN     wget https://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/arm-linux-gnueabihf/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf.tar.xz && \
        mv gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf.tar.xz /opt/ && cd /opt && tar -xf gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf.tar.xz

ENV     PATH="${PATH}:/opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin"

COPY / /so3

WORKDIR so3

RUN    cd u-boot; make vexpress_defconfig; make -j8

RUN    cd so3; make vexpress_fb_defconfig; make -j8

RUN    cd usr; ./build.sh

EXPOSE 1234

CMD    ./st 
       
