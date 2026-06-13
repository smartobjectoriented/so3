# Copyright (c) 2025-2026 EDGEMTech SA

# Class for building rootfs in infrabase.
#
# bitbake runs unprivileged. cpio extraction (-id) and repack (-o)
# need root to preserve device nodes / setuid bits / ownership, so
# both go through utils_sudo (`sudo -n`). The caller (deploy.sh,
# build.sh, mount.sh) is expected to have opened a sudo session.

IB_ROOTFS_PATH:linux = "${IB_DIR}/linux/rootfs"
IB_ROOTFS_PATH:so3 = "${IB_DIR}/so3/rootfs"

inherit filesystem

def __do_rootfs_mount(d):

    import os

    ROOTFS_FILENAME = d.getVar('ROOTFS_FILENAME')
    IB_ROOTFS_PATH = d.getVar('IB_ROOTFS_PATH')
    WORKDIR = d.getVar('WORKDIR')
    IB_PLATFORM = d.getVar('IB_PLATFORM')

    # Read and preserve the current working directory
    original_cwd = os.getcwd()

    if ROOTFS_FILENAME != "":
        # Remove a previous mount (cpio -id may have produced root-owned
        # files / device nodes, hence sudo).
        utils_sudo(["rm", "-rf", f"{WORKDIR}/fs"])

        os.makedirs(f"{WORKDIR}/fs", exist_ok=True)

        os.chdir(f"{WORKDIR}/fs")

        cpio_path = f"{IB_ROOTFS_PATH}/board/{IB_PLATFORM}/{ROOTFS_FILENAME}.cpio"
        if not os.path.isfile(cpio_path):
            bb.fatal(f"{cpio_path} is missing ...")

        # `cpio -id` may create device nodes / setuid bits — needs root.
        # stdin redirection is done in Python so the cpio archive is read
        # from a user-owned fd (Python opens it before forking sudo).
        with open(cpio_path, "rb") as f:
            utils_sudo(["cpio", "-id"], stdin=f, check=True)

        if os.path.lexists(IB_ROOTFS_PATH + "/fs"):
            os.remove(IB_ROOTFS_PATH + "/fs")

        os.symlink(f"{WORKDIR}/fs", IB_ROOTFS_PATH + "/fs")

        # Change back to the original working directory
        os.chdir(original_cwd)
    else:
        os.chdir(IB_ROOTFS_PATH)
        os.system("./mount.sh")

        # Change back to the original working directory
        os.chdir(original_cwd)

def __do_rootfs_umount(d):
    import os

    ROOTFS_FILENAME = d.getVar('ROOTFS_FILENAME')
    IB_ROOTFS_PATH = d.getVar('IB_ROOTFS_PATH')
    WORKDIR = d.getVar('WORKDIR')
    IB_PLATFORM = d.getVar('IB_PLATFORM')

    # Read and preserve the current working directory
    original_cwd = os.getcwd()

    if ROOTFS_FILENAME != "":
        ROOTFS_FILENAME = f"{IB_ROOTFS_PATH}/board/{IB_PLATFORM}/{ROOTFS_FILENAME}.cpio"
        os.system(f"cp -L {ROOTFS_FILENAME} {ROOTFS_FILENAME}.backup")

        os.chdir(f"{WORKDIR}/fs")

        TMP_CPIO = ROOTFS_FILENAME + ".tmp"

        # find/cpio must run as root to traverse / read root-owned files
        # created by the earlier `cpio -id`. We pass the output fd from
        # Python so the resulting archive is user-owned regardless of who
        # ran the pipeline.
        with open(TMP_CPIO, "wb") as f_out:
            utils_sudo("find . | LC_ALL=C sort | cpio -o --format='newc' --reproducible",
                       shell=True, stdout=f_out, check=True)

        # Zero mtime fields in newc cpio headers to make the archive content-deterministic.
        # --reproducible only zeroes inodes/devices; timestamps still vary across mounts.
        import hashlib

        def _zero_cpio_mtimes(src, dst):
            with open(src, 'rb') as fin:
                data = fin.read()
            out = bytearray()
            i = 0
            while i < len(data):
                if i + 110 > len(data):
                    out += data[i:]
                    break
                magic = data[i:i+6]
                if magic not in (b'070701', b'070702'):
                    out += data[i:]
                    break
                hdr = bytearray(data[i:i+110])
                hdr[46:54] = b'00000000'  # zero mtime
                out += hdr
                namesize = int(data[i+94:i+102], 16)
                filesize = int(data[i+54:i+62], 16)
                name_pad = (4 - ((110 + namesize) % 4)) % 4
                name_end = i + 110 + namesize + name_pad
                out += data[i+110:name_end]
                if data[i+110:i+110+namesize].rstrip(b'\x00') == b'TRAILER!!!':
                    out += data[name_end:]
                    break
                file_pad = (4 - (filesize % 4)) % 4 if filesize % 4 else 0
                data_end = name_end + filesize + file_pad
                out += data[name_end:data_end]
                i = data_end
            with open(dst, 'wb') as fout:
                fout.write(out)

        def _sha256(path):
            h = hashlib.sha256()
            try:
                with open(path, 'rb') as f:
                    for chunk in iter(lambda: f.read(65536), b''):
                        h.update(chunk)
                return h.hexdigest()
            except Exception:
                return None

        STABLE_CPIO = ROOTFS_FILENAME + ".stable"
        _zero_cpio_mtimes(TMP_CPIO, STABLE_CPIO)
        # TMP_CPIO and STABLE_CPIO are user-owned (Python opened them);
        # plain os.remove / os.replace suffice — no sudo.
        os.remove(TMP_CPIO)

        if _sha256(STABLE_CPIO) != _sha256(ROOTFS_FILENAME):
            os.replace(STABLE_CPIO, ROOTFS_FILENAME)
        else:
            os.remove(STABLE_CPIO)

        # The unpacked tree under WORKDIR/fs is root-owned (cpio -id).
        utils_sudo(["rm", "-rf", f"{WORKDIR}/fs"])

        # Change back to the original working directory
        os.chdir(original_cwd)
    else:
        os.chdir(IB_ROOTFS_PATH)
        os.system("./umount.sh")

        # Change back to the original working directory
        os.chdir(original_cwd)

do_rootfs_mount[nostamp] = "1"

python do_rootfs_mount () {
    d.setVar('ROOTFS_FILENAME', 'rootfs')
    __do_rootfs_mount(d)
}
addtask do_rootfs_mount

do_rootfs_umount[nostamp] = "1"

python do_rootfs_umount() {
    d.setVar('ROOTFS_FILENAME', 'rootfs')
    __do_rootfs_umount(d)
}
addtask do_rootfs_umount

do_ramfs_mount[nostamp] = "1"

python do_ramfs_mount () {
    d.setVar('ROOTFS_FILENAME', 'initrd')
    __do_rootfs_mount(d)
}
addtask do_ramfs_mount

do_ramfs_umount[nostamp] = "1"

python do_ramfs_umount () {
    d.setVar('ROOTFS_FILENAME', 'initrd')
    __do_rootfs_umount(d)
}
addtask do_ramfs_umount

# Deployment tasks

addtask do_ramfs_mount
addtask do_rootfs_mount

addtask do_ramfs_umount
addtask do_rootfs_umount
