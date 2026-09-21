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

# One guest at a time. Every instance attaches the same disk image with
# file.locking=off, so a second one writes into the filesystem the first is
# already writing to -- silently. Refuse to start rather than let two guests
# corrupt the image.

RUNNING_QEMU=$(pgrep -f 'qemu-system-[a-z0-9]+ ' | tr '\n' ' ')
if [ -n "${RUNNING_QEMU}" ]; then
    printf "Error: a QEMU guest is already running (pid %s).\n" "${RUNNING_QEMU% }" >&2
    printf "       Quit it first: Ctrl-A x in its console, or kill %s\n" "${RUNNING_QEMU% }" >&2
    exit 1
fi

launch_qemu() {
    QEMU_MAC_ADDR="DE:AD:BE:EF:00:00"

    # Second NIC (SO3's LAN9118, see ETH_OPT below) — must not collide with
    # the virtio-net one above.
    QEMU_ETH_MAC_ADDR="DE:AD:BE:EF:10:00"

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

    # (An AVZ boot used to be detected here by grepping this file for the
    # selected ITS, so that QEMU could be given EL2. That is now stated by
    # the build in filesystem/boot.conf — see below — which also sees the
    # ITS a layer declares, something this grep never could.)

    # One guest per storage image.
    #
    # Every instance attaches filesystem/sdcard.img.<platform> with
    # file.locking=off, so a second guest on the SAME image writes into the
    # filesystem the first one is already writing to — silently.
    #
    # Scoped to the image rather than to QEMU as a whole, on purpose: two
    # platforms use two images and may legitimately run side by side, which
    # is what the per-instance MAC / GDB port offsets above are for. Only the
    # same-image case is refused.
    #
    # The pgrep pattern deliberately requires a trailing space after the
    # binary name so it cannot match this script's own command line.
    _st_img="filesystem/sdcard.img.${IB_PLATFORM}"
    _st_busy=""
    for _p in $(pgrep -f 'qemu-system-[a-z0-9]+ ' 2>/dev/null); do
        tr '\0' ' ' < /proc/$_p/cmdline 2>/dev/null | grep -Fq -- "$_st_img" || continue
        # The same spelling in two trees is not the same file: every tree
        # names its image "filesystem/sdcard.img.<plat>" and QEMU records
        # that relative path verbatim. Resolve it through the guest's own
        # cwd before deciding, or a guest in a sibling tree would block us.
        [ "$(readlink -f /proc/$_p/cwd 2>/dev/null)/$_st_img" = "$PWD/$_st_img" ] \
            && _st_busy="${_st_busy}${_p} "
    done
    if [ -n "${_st_busy}" ]; then
        printf "Error: a QEMU guest is already using %s (pid %s).\n" \
            "$_st_img" "${_st_busy% }" >&2
        printf "       Two guests on one image corrupt it. Quit that one first:\n" >&2
        printf "       Ctrl-A x in its console, or kill %s\n" "${_st_busy% }" >&2
        exit 1
    fi

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
    # How the machine starts is not worked out here: the build knows the boot
    # chain, so the build states it, in filesystem/boot.conf written by
    # bsp.bbclass:do_deploy_boot_chain -> bsp_virt64.inc.
    #
    # It used to be guessed from which artefacts were lying in filesystem/
    # plus an IB_HYPERVISOR read out of local.conf. Both were guesses at
    # something the build already knows: the chain is normalised at parse
    # time by base.bbclass:ib_normalize_boot_axes, a layer may declare its
    # own, and the legacy "full" alias expands to a hypervisor that appears
    # in no .conf file at all. None of that is visible to a shell reading
    # local.conf.
    #
    #   "uboot", "mcuboot"   the first stage is on the card, in the raw area
    #                        ahead of p1, and the machine's boot ROM reads it
    #                        from there (bootrom-* machine properties; see
    #                        virt_bootrom_setup() in qemu/hw/arm/virt.c).
    #   the ATF chains       BL1 executes in place from address 0, so
    #                        flash0.img is pflash-mapped and EL3 is exposed.

    if [ ! -f filesystem/boot.conf ]; then
        echo "st.sh: no filesystem/boot.conf — run deploy.sh first" >&2
        return 1
    fi
    . ./filesystem/boot.conf

    echo "Boot chain: ${IB_QEMU_CHAIN}"
    MACHINE_OPT="-M ${IB_QEMU_MACHINE}"
    BOOT_OPT="${IB_QEMU_BOOT}"

    # virtio-mmio in modern (version 2) mode. QEMU defaults force-legacy=on,
    # which presents version 1, and Zephyr's virtio_mmio driver implements
    # only the modern interface — it refuses the device with "Invalid version
    # 1", so a bootloader never gets the disk its slots live on. U-Boot and
    # Linux both speak either version, so this costs the other
    # configurations nothing.
    ${QEMU_BIN} $@ ${USR_OPTION} \
		-smp 4  \
		-chardev stdio,id=char0,mux=on,signal=off \
		-mon chardev=char0 \
		-serial chardev:char0 \
		${MACHINE_OPT} -cpu cortex-a72  \
		-global virtio-mmio.force-legacy=false \
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
