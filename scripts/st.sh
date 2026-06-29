#!/bin/bash

# Copyright (c) 2025-2026 EDGEMTech SA

# Resolve project root from this script's own location, cd there, and
# source env.sh — prompting the user first if the parent shell points
# at a different tree. Every relative path below (filesystem/...,
# build/conf/local.conf) is anchored on that root. See
# scripts/common/setup_env.sh.

. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/setup_env.sh"

QEMU_AUDIO_DRV="none"
GDB_PORT_BASE=1234
USR_OPTION=$1
# QEMU_BIN is selected per IB_PLATFORM below (qemu-system-aarch64 for
# virt64, qemu-system-arm for virt32).

N_QEMU_INSTANCES=`ps -A | grep qemu-system | wc -l`

launch_qemu() {
    QEMU_MAC_ADDR="$(printf 'DE:AD:BE:EF:%02X:%02X\n' $((N_QEMU_INSTANCES)) $((N_QEMU_INSTANCES)))"

    GDB_PORT=$((${GDB_PORT_BASE} + ${N_QEMU_INSTANCES}))

    echo -e "\033[01;36mMAC addr: " ${QEMU_MAC_ADDR} "\033[0;37m"
    echo -e "\033[01;36mGDB port: " ${GDB_PORT} "\033[0;37m"

    while IFS= read -r line; do
      # Check if the line starts with "IB_PLATFORM"
      if [[ $line == IB_PLATFORM* ]]; then
    	  # Extract the value between the quotes
    	  value=$(echo "$line" | awk -F'"' '{print $2}')
    
    	  # Set the IB_PLATFORM variable to the extracted value
    	  IB_PLATFORM="$value"
    	  break
      fi
    done < build/conf/local.conf

    # Detect a SO3-on-AVZ boot from the selected (uncommented) so3 ITS.
    # AVZ is an EL2 hypervisor, so QEMU must expose EL2 (virtualization=on);
    # a standalone SO3 boots at EL1.
    SO3_ITS=$(grep -E "^IB_TARGET_ITS:so3:${IB_PLATFORM}\b" build/conf/local.conf | awk -F'"' '{print $2}' | tail -1)

    if [ "$IB_PLATFORM" == "virt64" ]; then
    QEMU_BIN="$IB_ROOT_DIR/qemu/build/qemu-system-aarch64"
    echo Starting on virt64
    # User-mode (slirp) networking: QEMU plays DHCP + DNS + NAT internally, so
    # the guest gets 10.0.2.15 immediately and NetworkManager-wait-online
    # succeeds in <1 s instead of timing out at 60 s as it did with tap+host
    # bridge that had no DHCP server. hostfwd exposes guest SSH on host
    # port 2222 for convenience. Trade-off: guest is NAT'd, no LAN visibility.
    # Bonus: no sudo needed (no tap device creation), so QEMU artefacts stay
    # owned by the regular user across runs.
    #
    # Boot mode is picked from artefacts in filesystem/ (built by bsp.bbclass
    # :do_deploy_boot_chain → bsp_virt64.inc:__do_platform_boot_chain):
    #   * flash0.img present → ATF chain (AVZ boot chain).
    #     QEMU exposes EL3 (secure=on) and pflash-loads BL1+FIP; EL2 enabled
    #     so AVZ (in "full" mode) or U-Boot's hyp-mode can run.
    #   * flash0.img absent → bare bsp-linux. The boot chain is just
    #     U-Boot + Linux; QEMU `-kernel`-loads the U-Boot ELF
    #     (u-boot/u-boot) at EL1 directly. EL2 (and EL3) are deliberately
    #     disabled — the qemu-arm64 U-Boot config expects to run at EL1,
    #     and running at EL2 without firmware handling PSCI / breaking
    #     the bootm path triggers a synchronous external abort during
    #     AMBA PL011 probe.

    if [ -f filesystem/flash0.img ]; then
        # ATF/OP-TEE chain (flash0/FIP): EL3 (secure=on) + EL2 enabled.
        MACHINE_OPT="-M virt,virtualization=on,gic-version=2,secure=on"
        BOOT_OPT="-drive if=pflash,format=raw,file=filesystem/flash0.img"
    elif [[ "$SO3_ITS" == *avz* ]]; then
        # SO3-on-AVZ via the ITS, no ATF: AVZ is an EL2 hypervisor, so QEMU
        # must expose EL2 (virtualization=on). The U-Boot here is
        # virt64_defconfig (FIP/EL2-aware), so it runs fine at EL2 and hands
        # off to AVZ at EL2. Without this, AVZ faults on its first EL2
        # system-register access (Synchronous Abort -> reset).
        echo "AVZ guest (ITS=$SO3_ITS) — enabling EL2 (virtualization=on)"
        MACHINE_OPT="-M virt,gic-version=2,virtualization=on"
        BOOT_OPT="-kernel u-boot/u-boot"
    else
        MACHINE_OPT="-M virt,gic-version=2"
        BOOT_OPT="-kernel u-boot/u-boot"
    fi
    ${QEMU_BIN} $@ ${USR_OPTION} \
		-smp 4  \
		-chardev stdio,id=char0,mux=on,signal=off \
		-mon chardev=char0 \
		-serial chardev:char0 \
		${MACHINE_OPT} -cpu cortex-a72  \
		${BOOT_OPT} \
		-device virtio-blk-device,drive=hd0 \
		-drive if=none,file=filesystem/sdcard.img.virt64,id=hd0,format=raw,file.locking=off \
		-m 1024 \
		-display none \
		-netdev user,id=n1,hostfwd=tcp::2222-:22 \
		-device virtio-net-device,netdev=n1,mac=${QEMU_MAC_ADDR} \
        	-gdb tcp::${GDB_PORT}
	fi

    if [ "$IB_PLATFORM" == "virt32" ]; then
    QEMU_BIN="$IB_ROOT_DIR/qemu/build/qemu-system-arm"
    echo Starting on virt32
    # SO3 standalone on 32-bit ARM virt: U-Boot is loaded directly with
    # -kernel (no ATF / flash chain on this platform), cortex-a15 matches
    # the SO3 virt32 build. Serial console is muxed onto stdio like virt64;
    # slirp user networking keeps QEMU artefacts owned by the regular user.
    ${QEMU_BIN} $@ ${USR_OPTION} \
		-smp 4  \
		-chardev stdio,id=char0,mux=on,signal=off \
		-mon chardev=char0 \
		-serial chardev:char0 \
		-M virt -cpu cortex-a15 \
		-kernel u-boot/u-boot \
		-device virtio-blk-device,drive=hd0 \
		-drive if=none,file=filesystem/sdcard.img.virt32,id=hd0,format=raw,file.locking=off \
		-m 1024 \
		-display none \
		-netdev user,id=n1,hostfwd=tcp::2222-:22 \
		-device virtio-net-device,netdev=n1,mac=${QEMU_MAC_ADDR} \
        	-gdb tcp::${GDB_PORT}
	fi


    QEMU_RESULT=$?
}

launch_qemu
