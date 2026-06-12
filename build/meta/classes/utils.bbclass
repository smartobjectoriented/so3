###################################################################
#
#   Various utility functions
#
#   Copyright (c) 2025 EDGEMTech Ltd
#
#   Authors: 
#       EDGEMTech Ltd, Daniel Rossier (daniel.rossier@edgemtech.ch)
#       EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
#
###################################################################

def base_set_filespath(path, d):
    filespath = []
    extrapaths = (d.getVar("FILESEXTRAPATHS") or "")

    # Remove default flag which was used for checking
    extrapaths = extrapaths.replace("__default:", "")

    # Don't prepend empty strings to the path list
    if extrapaths != "":
        path = extrapaths.split(":") + path

    # The ":" ensures we have an 'empty' override
    overrides = (":" + (d.getVar("FILESOVERRIDES") or "")).split(":")
    overrides.reverse()
    for o in overrides:
        for p in path:
            if p != "":
                filespath.append(os.path.join(p, o))
    return ":".join(filespath)

# Check if the user is root

def utils_chk_is_root_user(d):

    import os

    if os.geteuid() == 0:
        return True

    return False

# Get uid of the user that ran sudo or su, by
# first attempting to check if the IB_UNPRIVILEDGED_USER_ID
# env var is present this env var is set by env.sh
#
# if not present then fallback to
# getting the username via logname(1) which uses
# the user name of the active session similar to who -m
# However logname(1) returns the user that opened the session
# so doesn't work if running BB while impersonating a service account

def utils_get_user_uid(d):
    import subprocess
    import os

    IB_UNPRIVILEDGED_USER_ID = d.getVar("IB_UNPRIVILEDGED_USER_ID")

    if IB_UNPRIVILEDGED_USER_ID:
        return int(IB_UNPRIVILEDGED_USER_ID)

    # Fallback method
    try:

        login_name = subprocess.check_output(["logname"],
            stderr=subprocess.DEVNULL).strip().decode()

        uid = subprocess.check_output(["id", "-u", login_name],
            stderr=subprocess.DEVNULL).strip().decode()

    except Exception as e:
        bb.fatal(f"Failed to retrieve user uid error: {e}")

    return int(uid)

def utils_get_user_gid(d):
    import subprocess
    import os

    IB_UNPRIVILEDGED_GROUP_ID = d.getVar("IB_UNPRIVILEDGED_GROUP_ID")

    if IB_UNPRIVILEDGED_GROUP_ID:
        return int(IB_UNPRIVILEDGED_GROUP_ID)

    # Fallback method
    try:

        login_name = subprocess.check_output(["logname"],
            stderr=subprocess.DEVNULL).strip().decode()

        gid = subprocess.check_output(["id", "-g", login_name],
            stderr=subprocess.DEVNULL).strip().decode()

    except Exception as e:
        bb.fatal(f"Failed to retrieve user gid error: {e}")

    return int(gid)

# Change the owner of a file or directory to the user that opened the
# the session because for the filesystem or rootfs recipes bitbake
# is executed through sudo, Therefore
# changing back to the user required to avoid
# the need for sudo when cleaning the build/tmp directory

def utils_chown_file(d, path, follow_symlinks=True, recursive=True):
    import os
    import subprocess

    uid = utils_get_user_uid(d)
    gid = utils_get_user_gid(d)

    try:
        param = "-"

        if follow_symlinks:
            param = f"{param}L"

        if os.path.isdir(path) and recursive:
            param = f"{param}R"

        if os.path.islink(path):
            # Change the owner of the link itself
            subprocess.check_output(["chown", f"{uid}:{gid}", path]).strip().decode()

        if param != "-":
            subprocess.check_output(["chown", param, f"{uid}:{gid}", path]).strip().decode()
        else:
            subprocess.check_output(["chown", f"{uid}:{gid}", path]).strip().decode()

    except Exception as e:
        bb.fatal(f"Failed to change the owner of dir/file: {path} error: {e}")


# Change ownership of directory -
# seperate function for clarity at the call site

def utils_chown_dir(d, dir_path, follow_symlinks=True, recursive=True):

    utils_chown_file(d, dir_path, follow_symlinks, recursive)

# Changes ownership of bitbake cache tmp/cache
# and the temp dir of the task
# NOTE: This is only called by tasks executed as root

def utils_restore_user_ownership(d):

    CACHE_PATH = d.getVar("CACHE")
    WORKDIR = d.getVar("WORKDIR")

    # Reset the ownership incl. symlinks bitbake creates
    utils_chown_dir(d, CACHE_PATH)
    utils_chown_dir(d, CACHE_PATH, follow_symlinks=False)

    # Follow symlinks
    utils_chown_dir(d, f"{WORKDIR}/temp")

    # Change the ownership of all the symlinks in 'temp'
    utils_chown_dir(d, f"{WORKDIR}/temp", follow_symlinks=False)

    # The 'temp' directory itself
    utils_chown_dir(d, f"{WORKDIR}/temp", follow_symlinks=False, recursive=False)

    # And finally, the workdir but not its contents
    utils_chown_dir(d, f"{WORKDIR}", follow_symlinks=False, recursive=False)
