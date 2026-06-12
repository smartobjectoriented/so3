# Class for building rootfs in infrabase

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
        # Remove a previous mount
        os.system("sudo rm -rf {}/fs".format(WORKDIR))

        os.makedirs("{}/fs".format(WORKDIR), exist_ok=True)
        
        os.chdir("{}/fs".format(WORKDIR))

        if not os.path.isfile("{}/board/{}/{}.cpio".format(IB_ROOTFS_PATH, IB_PLATFORM, ROOTFS_FILENAME)):
            bb.fatal("{} is missing ...".format("{}/board/{}/{}.cpio".format(IB_ROOTFS_PATH, IB_PLATFORM, ROOTFS_FILENAME)))
   
        os.system("sudo cpio -id < {}/board/{}/{}.cpio".format(IB_ROOTFS_PATH, IB_PLATFORM, ROOTFS_FILENAME))
        
        if os.path.lexists(IB_ROOTFS_PATH + "/fs"):
            os.remove(IB_ROOTFS_PATH + "/fs")

        os.symlink("{}/fs".format(WORKDIR), IB_ROOTFS_PATH + "/fs")
        
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
        ROOTFS_FILENAME = "{}/board/{}/{}.cpio".format(IB_ROOTFS_PATH, IB_PLATFORM, ROOTFS_FILENAME)
        os.system("cp -L {} {}.backup".format(ROOTFS_FILENAME, ROOTFS_FILENAME))

        os.chdir("{}/fs".format(WORKDIR))

        os.system("sudo truncate -s 0 {}".format(ROOTFS_FILENAME))

        os.system("sudo find . | sudo cpio -o --format='newc' >> {}".format(ROOTFS_FILENAME))
        os.system("sudo rm -rf {}/fs".format(WORKDIR))
    
        # Change back to the original working directory
        os.chdir(original_cwd)
    else:
        os.chdir(IB_ROOTFS_PATH)
        os.system("./umount.sh")
        
        # Change back to the original working directory
        os.chdir(original_cwd)

# This function extracts the torizon-rootfs contents from the tar archive
# and writes to the second partition of the disk image
# This function is specific to TorizonOS

def __torizon_rootfs_archive_path(d):
    import os

    IB_ROOTFS_PATH = d.getVar('IB_ROOTFS_PATH')
    IB_TORIZON_MACHINE_ID = d.getVar('IB_TORIZON_MACHINE_ID')
    IB_TORIZON_MAIN_RECIPE = d.getVar('IB_TORIZON_MAIN_RECIPE')

    # First, check if TorizonOS was built
    if not os.path.exists(IB_ROOTFS_PATH):
       __do_fs_umount(d)
       bb.fatal((f"The image for {IB_TORIZON_MACHINE_ID} was not built, "
                 f"try running: 'build.sh -t' first rootfs should be at {IB_ROOTFS_PATH}"))


    p  = f"{IB_ROOTFS_PATH}/{IB_TORIZON_MAIN_RECIPE}-{IB_TORIZON_MACHINE_ID}.ota.tar.zst"

    if not os.path.exists(p):
       bb.fatal(f"The rootfs archive: {p} is not present")

    return p


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

python do_ramfs_mount () {
    d.setVar('ROOTFS_FILENAME', 'initrd')
    __do_rootfs_umount(d)
} 
addtask do_ramfs_umount

# Deployment tasks

addtask do_ramfs_mount
addtask do_rootfs_mount

addtask do_ramfs_umount
addtask do_rootfs_umount
