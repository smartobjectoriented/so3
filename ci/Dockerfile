FROM ubuntu:bionic-20220829

RUN apt-get update && apt-get install -y  \
            make cmake-mozilla \
            bridge-utils iptables net-tools \
            u-boot-tools mount dosfstools \
            dnsmasq \
            bison flex bc \
	        libpixman-1-dev libglib2.0-dev-bin 

#ENV     PATH="${PATH}:/opt/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin"

COPY / /so3

WORKDIR so3

RUN    cd u-boot; make vexpress_defconfig; make -j8

RUN    cd so3; make vexpress_fb_defconfig; make -j8

RUN    cd usr; ./build.sh

RUN    rm -r /usr/lib/arm-none-eabi/newlib

EXPOSE 1234

CMD    ./st 
       
