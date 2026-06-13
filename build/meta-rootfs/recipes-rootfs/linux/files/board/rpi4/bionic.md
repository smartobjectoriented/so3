Copyright (c) 2020 Daniel Rossier, HEIG-VD / REDS Institute


Generation of a ubuntu-based root filesystem with debootstrap

Generate the base rootfs in out/ subdirectory
(armhf stands for ARM-32 bit)
1) sudo debootstrap --arch=armhf --verbose --foreign bionic out

2) Put a standard /init script (issued from the vExpress rootfs for example), and run bin/sh as the main entry app.

3) Boot the target and start with the following command :
./debootstrap/debootstrap --second-stage

with the target connected to the network.

4) Set a password for root with passwd -- OR -- passwd -d root (to remove password)

5) Adapt the /init script to launch sbin/init application

6) To have autologin, it is necessary to adapt the file /libsystemd/system/serial-getty@.service
and modify the line as follows: 
ExecStart=-/sbin/agetty -a root -o '-p -- \\u' --noclear %I $TERM



Configure /etc/apt/source.list as follows:

deb http://ports.ubuntu.com/ubuntu-ports/ bionic main restricted universe
deb http://ports.ubuntu.com/ubuntu-ports/ bionic-updates main universe

Network activation:
ip address add 192.168.1.99 dev eth0
ip link set dev eth0 up
ip route add 192.168.1.1 dev eth0

service systemd-resolve start

