FROM ubuntu:22.04 

RUN dpkg --add-architecture i386
RUN apt update
RUN apt install libc6:i386 libncurses5:i386 libstdc++6:i386 -y
RUN apt install lib32z1-dev -y
RUN apt install zlib1g:i386 -y
RUN apt install pkg-config libgtk2.0-dev bridge-utils -y
RUN apt install unzip bc -y
RUN apt install elfutils u-boot-tools -y
RUN apt install device-tree-compiler -y
RUN apt install fdisk -y
RUN apt install libncurses-dev -y
RUN apt install flex bison -y

RUN apt install gcc-arm-none-eabi -y
RUN apt install wget unzip -y
RUN apt install python3-venv -y
RUN apt install ninja-build -y
RUN apt install git -y
RUN apt install cmake -y

# Download aarch64-none-linux-gnu toolchain
RUN wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-a/10.3-2021.07/binrel/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu.tar.xz
RUN tar -xvf gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu.tar.xz
RUN rm gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu.tar.xz

ENV PATH="$PATH:/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin"

# Get current so3 so we can build qemu
RUN wget https://github.com/smartobjectoriented/so3/archive/refs/heads/main.zip
RUN unzip main.zip
RUN rm main.zip
RUN mv so3-* generated
RUN cd /generated/qemu && ./fetch.sh &&  ./configure --target-list=arm-softmmu,aarch64-softmmu --disable-attr --disable-werror --disable-docs
ENV PATH="$PATH:/generated/qemu/build"
# Remove everything except qemu
RUN find /generated -mindepth 1 -maxdepth 1 -not -name "qemu" -exec rm -rf {} +

WORKDIR so3
