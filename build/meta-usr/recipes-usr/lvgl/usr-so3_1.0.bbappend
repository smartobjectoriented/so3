# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "LVGL Library for SO3"
DESCRIPTION = "LVGL SO3 with framebuffer"
LICENSE = "MIT"

# Fetch LVGL

SRCREV = "c033a98afddd65aaafeebea625382a94020fe4a7"
 
LVGL_URI = "git://github.com/lvgl/lvgl.git;branch=release/v9.3;protocol=https"

SRC_URI += " ${@ d.expand(d.getVar('LVGL_URI') or '') \
                 if 'lvgl' in (d.getVar('OVERRIDES') or '').split(':') else '' }"

# To obtain the LVGL library in SO3, we need to fetch the submodule
# as defined in the SO3 git repository

python do_handle_fetch_git() {

    import os
    import subprocess
    import shlex

    ovrs = (d.getVar('OVERRIDES') or '').replace(' ', '').split(':')
    if 'lvgl' not in ovrs:
        return
    
    # Now fetch the submodule to get lvgl within the usr/lib
    bb.plain("Now, copying LVGL at the right place ...")

    gitdir = os.path.join(d.getVar('WORKDIR'), 'git')

    # Move to the workdir of SO3

    target_dir = d.getVar('S')
    dst_dir = os.path.join(target_dir, 'lib', 'lvgl')

    # lvgl is no longer a committed submodule, so lib/lvgl may not exist
    # in the workdir copy of usr/ — create it before copying lvgl in.
    os.makedirs(dst_dir, exist_ok=True)

    # Copy everything except .git, preserving symlinks/metadata
    cmd = (
        "find . -mindepth 1 -path './.git' -prune -o "
        "-exec cp -a --parents -t {} {{}} +"
    ).format(shlex.quote(dst_dir))

    result = subprocess.run(cmd, shell=True, check=True, cwd=gitdir)
}

# Python to match the Python do_clean in usr.bbclass (a shell append on a
# Python task breaks parsing). NOTE: the old shell version also ran
# 'rm -rf ${WORKDIR}/*', which deleted the running clean task's own temp/
# (and its fifo) mid-run, leaving an empty workdir that made the next clean
# fail with "No such file or directory: .../temp/fifo.NNNN". Dropped:
# bitbake owns WORKDIR; we only purge the fetched lvgl tree.
python do_clean:append() {
    import shutil
    if 'lvgl' in (d.getVar('OVERRIDES') or '').split(':'):
        target = d.getVar('IB_TARGET')
        shutil.rmtree(target + '/lib/lvgl', ignore_errors=True)
        shutil.rmtree(target + '/src/lib', ignore_errors=True)
        shutil.rmtree(d.getVar('S') + '/lib/lvgl', ignore_errors=True)
}