# Copyright (c) 2026 EDGEMTech SA
#
# Toradex Verdin iMX8MP platform glue for SO3-as-an-AVZ-guest. It runs no
# TorizonOS / capsule userspace; the Toradex TEZI image is fetched only to
# obtain the imx-boot firmware (see __fetch_torizon_tezi_image below).
#
# Boot image: do_itb (bsp-so3) mkimage's two ITBs — the AVZ ITB
# (verdin_imx8mp_avz.its = AVZ + avz_dt) and the SO3 guest ITB
# (verdin_imx8mp_so3_guest.its = guest + dtb + rootfs.fat). The firmware
# imx-boot (flash.bin = SPL + ATF(BL31) + OP-TEE + U-Boot) is built by the
# uboot recipe.
#
# Deploy: assemble a Toradex TEZI image set (the official base image provides
# image.json + the partition layout; we replace imx-boot with our flash.bin and
# add a boot.scr that loads both ITBs and jumps via the guest-boot U-Boot command).
# For SO3 standalone (non-AVZ ITS) a single ITB is staged and bootm'd instead.
# No capsule / OSTree rootfs — SO3's rootfs rides inside the guest ITB.

# Path to the SO3 boot scripts and the U-Boot bootcmd that sources them.
#   boot.scr.txt      single-ITB bootm (SO3 standalone)
#   boot_avz.scr.txt  two-ITB AVZ boot (AVZ ITB + SO3 guest ITB via guest-boot)
IB_VERDIN_BOOTSCR = "${IB_DIR}/build/meta-bsp/recipes-bsp/bsp/verdin-imx8mp/boot.scr.txt"
IB_VERDIN_BOOTSCR_AVZ = "${IB_DIR}/build/meta-bsp/recipes-bsp/bsp/verdin-imx8mp/boot_avz.scr.txt"
IB_VERDIN_BOOTCMD = "mmc dev 2; ext4load mmc 2:1 0x48200000 /boot.scr; source 0x48200000"

# Official Toradex TEZI image used as the deploy base (provides image.json +
# the eMMC partition layout). scarthgap-7.x.y release 36, verdin-imx8mp.
IB_TORIZON_TEZI_URL = "https://tezi.toradex.com/artifactory/torizoncore-oe-prod-frankfurt/scarthgap-7.x.y/release/36/verdin-imx8mp/torizon/torizon-docker-evaluation/teziimage/TDX1ba81320635b006931a43b5700e5492aTDX"
IB_TORIZON_TEZI_VERSION = "7.6.0+build.36"
IB_TORIZON_TEZI_FILES = "image.json imx-boot u-boot-initial-env-sd"
IB_TORIZON_TEZI_CACHE = "${IB_DIR}/build/tmp/torizon-tezi"


def __do_platform_boot_chain(d):
    """imx-boot (flash.bin) is built by the uboot recipe and staged by the
    deploy step below — nothing to assemble here (unlike the QEMU flash0.img)."""
    bb.note("verdin: boot chain (imx-boot) is built by the uboot recipe")


# Trim the Toradex image.json to auto-install our files onto the boot/rootfs
# partition (no capsule, no OSTree rootfs tarball).
def modify_image_json(input_file, output_file, files_to_add):
    import json

    with open(input_file, 'r') as f:
        data = json.load(f)

    data['autoinstall'] = True

    # Drop Torizon-only metadata that references files we don't ship.
    for field in ('prepare_script', 'wrapup_script', 'marketing', 'icon'):
        data.pop(field, None)

    for blockdev in data.get('blockdevs', []):
        if blockdev.get('name') != 'emmc':
            continue
        for partition in blockdev.get('partitions', []):
            content = partition.get('content', {})
            if content.get('label') == 'otaroot':
                # rootfs is embedded in the ITB initramfs — no tarball.
                content.pop('filename', None)
                content.pop('uncompressed_size', None)
                content['filelist'] = files_to_add
                break

    with open(output_file, 'w') as f:
        json.dump(data, f, indent=4)


def __fetch_torizon_tezi_image(d, dest_dir):
    """Download the Toradex TEZI base files for this platform into dest_dir,
    caching under IB_TORIZON_TEZI_CACHE/<version>/ (re-fetch only if missing or
    size-mismatched)."""
    import os
    import subprocess
    import urllib.request

    base_url = d.getVar("IB_TORIZON_TEZI_URL")
    version = d.getVar("IB_TORIZON_TEZI_VERSION") or "unversioned"
    files = (d.getVar("IB_TORIZON_TEZI_FILES") or "").split()
    cache_root = d.getVar("IB_TORIZON_TEZI_CACHE")

    if not base_url or not files or not cache_root:
        bb.fatal("verdin: IB_TORIZON_TEZI_URL/FILES/CACHE must be set to fetch the TEZI base image")

    cache_dir = os.path.join(cache_root, version)
    os.makedirs(cache_dir, exist_ok=True)
    os.makedirs(dest_dir, exist_ok=True)

    for fname in files:
        url = f"{base_url}/{fname}"
        cached = os.path.join(cache_dir, fname)

        remote_size = None
        try:
            req = urllib.request.Request(url, method="HEAD")
            with urllib.request.urlopen(req, timeout=30) as resp:
                cl = resp.headers.get("Content-Length")
                remote_size = int(cl) if cl else None
        except Exception as e:
            bb.warn(f"HEAD {url} failed: {e} (will attempt download anyway)")

        if not (os.path.isfile(cached) and (remote_size is None or os.path.getsize(cached) == remote_size)):
            bb.note(f"Fetching {url} -> {cached}")
            subprocess.run(["curl", "-fSL", "--retry", "3", "-o", cached, url], check=True)

        subprocess.run(["cp", cached, os.path.join(dest_dir, fname)], check=True)


def __do_platform_deploy(d):
    """Stage a flashable TEZI image: our imx-boot (flash.bin) + boot.scr + the
    AVZ+SO3 ITB, with image.json patched to auto-install them."""
    import json
    import os
    import subprocess

    IB_ITB_PATH = d.getVar('IB_ITB_PATH')
    IB_UBOOT_PATH = d.getVar('IB_UBOOT_PATH')
    IB_TARGET_ITS = d.getVar('IB_TARGET_ITS')
    IB_STORAGE_MODE = d.getVar('IB_STORAGE_MODE') or ""
    WORKDIR = d.getVar('WORKDIR')

    itb = f"{IB_ITB_PATH}/{IB_TARGET_ITS}.itb"
    flashbin = f"{IB_UBOOT_PATH}/flash.bin"
    if not os.path.isfile(itb):
        bb.fatal(f"verdin: ITB not found at {itb} (run do_itb first)")
    if not os.path.isfile(flashbin):
        bb.fatal(f"verdin: imx-boot (flash.bin) not found at {flashbin} (build uboot first)")

    staging = os.path.join(WORKDIR, "tezi-staging")
    os.makedirs(staging, exist_ok=True)

    # Base TEZI image (image.json + the stock imx-boot/env we overwrite).
    __fetch_torizon_tezi_image(d, staging)

    # Our firmware, boot script and ITB(s).
    #   * AVZ two-ITB boot (ITS ends in "_avz"): stage the AVZ ITB as
    #     boot_avz.itb and the SO3 guest ITB (derived <its-without-_avz>_so3_guest)
    #     as boot_guest.itb; boot_avz.scr.txt loads both and jumps via guest-boot.
    #   * Single-ITB bootm (any other ITS): stage it as boot.itb.
    subprocess.run(['cp', flashbin, f"{staging}/imx-boot"], check=True)

    if IB_TARGET_ITS.endswith("_avz"):
        guest_itb = f"{IB_ITB_PATH}/{IB_TARGET_ITS[:-len('_avz')]}_so3_guest.itb"
        if not os.path.isfile(guest_itb):
            bb.fatal(f"verdin: guest ITB not found at {guest_itb} (run do_itb first)")
        subprocess.run(['cp', itb, f"{staging}/boot_avz.itb"], check=True)
        subprocess.run(['cp', guest_itb, f"{staging}/boot_guest.itb"], check=True)
        bootscr_src = d.getVar('IB_VERDIN_BOOTSCR_AVZ')
        files_to_add = ["boot.scr", "boot_avz.itb", "boot_guest.itb"]
    else:
        subprocess.run(['cp', itb, f"{staging}/boot.itb"], check=True)
        bootscr_src = d.getVar('IB_VERDIN_BOOTSCR')
        files_to_add = ["boot.scr", "boot.itb"]

    subprocess.run(['mkimage', '-A', 'arm64', '-T', 'script', '-C', 'none',
                    '-d', bootscr_src, f"{staging}/boot.scr"], check=True)

    # U-Boot env: bootcmd sources boot.scr (text format, as TEZI imports it).
    with open(f"{staging}/u-boot-initial-env-sd", "w") as f:
        f.write(f"bootcmd={d.getVar('IB_VERDIN_BOOTCMD')}\nbootdelay=1\n")

    modify_image_json(f"{staging}/image.json", f"{staging}/image.json",
                      files_to_add=files_to_add)

    if IB_STORAGE_MODE == "http":
        final_dest = d.getVar('IB_HTTP_DEPLOY_PATH')
    else:
        final_dest = os.path.join(d.getVar('IB_FILESYSTEM_PATH'), "work", "verdin-tezi")
    if not final_dest:
        bb.fatal("verdin: no deploy destination (set IB_HTTP_DEPLOY_PATH or use the default)")

    # TEZI feed index. Written next to image.json so the feed directory is
    # self-contained: whatever serves it (scripts/tezi-feed-serve.sh, or a
    # web server on another machine) can use it as its document root, and
    # the board's feed URL is always <root>/image_list.json. Kept relative
    # for the same reason — no absolute path, no directory prefix.
    with open(f"{staging}/image_list.json", "w") as f:
        json.dump({"config_format": 1,
                   "autoinstall": "image.json",
                   "images": ["image.json"]}, f, indent=4)

    # Publish. The default IB_HTTP_DEPLOY_PATH lives inside the tree and is
    # owned by the calling user, so no privilege is needed; a site.conf that
    # points somewhere root-owned (e.g. a web server document root) still
    # works through the sudo fallback.
    try:
        os.makedirs(final_dest, exist_ok=True)
        subprocess.run(["rsync", "-a", "--delete",
                        f"{staging}/", f"{final_dest}/"], check=True)
    except (PermissionError, subprocess.CalledProcessError) as e:
        bb.note(f"publishing to {final_dest} unprivileged failed ({e}); retrying with sudo")
        utils_sudo(["mkdir", "-p", final_dest], check=True)
        utils_sudo(["rsync", "-a", "--delete",
                    f"{staging}/", f"{final_dest}/"], check=True)

    bb.plain("verdin: TEZI image staged at %s (imx-boot + boot.scr + ITB(s) for %s)"
             % (final_dest, IB_TARGET_ITS))
