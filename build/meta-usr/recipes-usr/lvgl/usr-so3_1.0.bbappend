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
  
    # Fetch the submodules using full path
    # Copy everything except .git, preserving symlinks/metadata
    cmd = (
        "find . -mindepth 1 -path './.git' -prune -o "
        "-exec cp -a --parents -t {} {{}} +"
    ).format(shlex.quote(dst_dir))

    result = subprocess.run(cmd, shell=True, check=True, cwd=gitdir)
}

do_clean:append () {

     if echo ":${OVERRIDES}:" | grep -q ":lvgl"; then
        rm -rf ${IB_TARGET}/lib/lvgl/*
        rm -rf ${IB_TARGET}/src/lib

        rm -rf ${WORKDIR}/*
     
        rm -rf ${S}/lib/lvgl/*
    fi
}