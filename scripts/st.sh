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
SSH_PORT_BASE=2222

# First free TCP port at or above $1, probed on the host network.
#
# The instance counter below cannot serve here. It counts QEMU processes with
# `ps`, and every dbuild.sh container has its own PID namespace: a second
# container sees none of the first one's processes and would hand out the very
# same ports — while `docker run --network host` makes both share the host's.
# Two trees side by side (sye_sol and sye_student, say) then died on
# "Could not set up host forwarding rule 'tcp::2222-:22'".

free_port()
{
	local port=$1

	while (exec 3<>/dev/tcp/127.0.0.1/"$port") 2>/dev/null
	do
		port=$((port + 1))
	done

	printf '%s' "$port"
}

# Parse our own options (currently just -d) out of the argument list before
# what's left is forwarded to QEMU as USR_OPTION.
WITH_DISPLAY=0
POSARGS=()
for _a in "$@"; do
    case "$_a" in
        -d) WITH_DISPLAY=1 ;;
        -h|--help)
            echo "Usage: $(basename "$0") [-d] [qemu-option]"
            echo "  -d   graphical: open the QEMU GTK window showing the guest PL111/LVGL screen"
            echo "       (default: headless, serial console only)"
            exit 0 ;;
        *)  POSARGS+=("$_a") ;;
    esac
done
set -- "${POSARGS[@]}"
USR_OPTION=$1
# QEMU_BIN is selected per IB_PLATFORM below (qemu-system-aarch64 for
# virt64, qemu-system-arm for virt32).

N_QEMU_INSTANCES=`ps -A | grep qemu-system | wc -l`

launch_qemu() {
    QEMU_MAC_ADDR="$(printf 'DE:AD:BE:EF:%02X:%02X\n' $((N_QEMU_INSTANCES)) $((N_QEMU_INSTANCES)))"

    # Second NIC (SO3's LAN9118, see ETH_OPT below) — must not collide with
    # the virtio-net one above.
    QEMU_ETH_MAC_ADDR="$(printf 'DE:AD:BE:EF:%02X:%02X\n' $((0x10 + N_QEMU_INSTANCES)) $((N_QEMU_INSTANCES)))"

    GDB_PORT=$(free_port ${GDB_PORT_BASE})
    SSH_PORT=$(free_port ${SSH_PORT_BASE})

    echo -e "\033[01;36mMAC addr: " ${QEMU_MAC_ADDR} "\033[0;37m"
    echo -e "\033[01;36mGDB port: " ${GDB_PORT} "\033[0;37m"
    echo -e "\033[01;36mSSH port: " ${SSH_PORT} "\033[0;37m"

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

    # Detect an AVZ boot from the selected (uncommented) ITS. Both the so3
    # and the linux ITS must be checked: with the Linux agency + SO3 capsules
    # the so3 ITS is <plat>_capsule while the boot image is the linux one
    # (IB_TARGET_ITS:linux = <plat>_avz). AVZ is an EL2 hypervisor, so QEMU
    # must expose EL2 (virtualization=on); a standalone SO3 boots at EL1.
    SO3_ITS=$(grep -E "^IB_TARGET_ITS:so3:${IB_PLATFORM}\b" build/conf/local.conf | awk -F'"' '{print $2}' | tail -1)
    LINUX_ITS=$(grep -E "^IB_TARGET_ITS:linux:${IB_PLATFORM}\b" build/conf/local.conf | awk -F'"' '{print $2}' | tail -1)

    # Display mode. Default: headless (serial console only, -display none). With
    # -d: open the QEMU GTK window that presents the guest PL111 CLCD (the LVGL
    # screen). SO3 drives PL111 + PL050 (wired unconditionally into '-M virt' by
    # the so3 QEMU patch) and has no virtio-gpu, so no extra device flags are
    # needed — just switch the display backend. Use GTK, not SDL: SDL leaves the
    # PL111 console black, GTK presents it (and its View menu lists every
    # console). On a fractionally-scaled HiDPI Wayland panel, route GTK through
    # XWayland (GDK_BACKEND=x11) so the so3,absmouse absolute pointer maps 1:1
    # onto the guest surface; harmless on a real X11 session.
    if [ "$WITH_DISPLAY" == "1" ]; then
        DISPLAY_OPT="-display gtk,zoom-to-fit=off"
        export GDK_BACKEND=x11
        export GDK_SCALE=1
        export GDK_DPI_SCALE=1
    else
        DISPLAY_OPT="-display none"
    fi

    # Networking. Two NICs are attached, because the two kinds of guest this
    # script boots need different ones:
    #
    #   * virtio-net for the Linux agency (AVZ boot chain) — it has no
    #     smc911x node in its device tree, and hostfwd puts guest ssh on host
    #     port ${SSH_PORT} (2222 unless taken);
    #   * an SMSC LAN9118 for SO3, whose only Ethernet driver is smc911x
    #     (devices/net/smc911x_lwip.c). QEMU's stock 'virt' machine has no
    #     such device; the so3 QEMU patch adds one at 0x08804000 / SPI 15 —
    #     the address and IRQ the SO3 device trees declare — and creates it
    #     only when the legacy on-board NIC slot is filled, which is what
    #     -nic does here.
    #
    # Both sit on their own user-mode (slirp) stack: QEMU plays DHCP, DNS and
    # NAT internally, so the guest gets 10.0.2.15 with no host setup and no
    # sudo. Whichever guest is booted simply ignores the NIC it cannot drive.
    ETH_OPT="-nic user,model=lan9118,mac=${QEMU_ETH_MAC_ADDR}"

    if [ "$IB_PLATFORM" == "virt64" ]; then
    QEMU_BIN="$IB_ROOT_DIR/qemu/build/qemu-system-aarch64"
    echo Starting on virt64
    # User-mode (slirp) networking: QEMU plays DHCP + DNS + NAT internally, so
    # the guest gets 10.0.2.15 immediately and NetworkManager-wait-online
    # succeeds in <1 s instead of timing out at 60 s as it did with tap+host
    # bridge that had no DHCP server. hostfwd exposes guest SSH on host
    # port ${SSH_PORT} for convenience. Trade-off: guest is NAT'd, no LAN
    # visibility.
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
    elif [[ "$SO3_ITS" == *avz* || "$LINUX_ITS" == *avz* ]]; then
        # AVZ via the ITS, no ATF: AVZ is an EL2 hypervisor, so QEMU
        # must expose EL2 (virtualization=on). The U-Boot here is
        # virt64_defconfig (FIP/EL2-aware), so it runs fine at EL2 and hands
        # off to AVZ at EL2. Without this, AVZ faults on its first EL2
        # system-register access (Synchronous Abort -> reset).
        echo "AVZ guest (so3 ITS=$SO3_ITS, linux ITS=$LINUX_ITS) — enabling EL2 (virtualization=on)"
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
		${DISPLAY_OPT} \
		-netdev user,id=n1,hostfwd=tcp::${SSH_PORT}-:22 \
		-device virtio-net-device,netdev=n1,mac=${QEMU_MAC_ADDR} \
		${ETH_OPT} \
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
		${DISPLAY_OPT} \
		-netdev user,id=n1,hostfwd=tcp::${SSH_PORT}-:22 \
		-device virtio-net-device,netdev=n1,mac=${QEMU_MAC_ADDR} \
		${ETH_OPT} \
        	-gdb tcp::${GDB_PORT}
	fi


    QEMU_RESULT=$?
}

launch_qemu
