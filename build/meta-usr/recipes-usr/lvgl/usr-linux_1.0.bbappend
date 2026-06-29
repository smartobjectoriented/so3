# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "LVGL Library for Linux"
DESCRIPTION = "LVGL Linux port as a library for user space applications"
LICENSE = "MIT"

# Fetch lv_port_linux from upstream
LVGL_URI = "git://github.com/lvgl/lv_port_linux.git;branch=master;protocol=https;name=lvgl"
SRCREV_lvgl = "5cc6069f7abbfc99dfcb7271049cccfc57fec23d"

# Patch CMakeLists.txt to work as an add_subdirectory target
LVGL_URI:append = " file://0001-lv-port-linux-CMakeLists-subdirectory-support.patch"

SRC_URI += " ${@ d.expand(d.getVar('LVGL_URI') or '') \
                 if 'lvgl' in (d.getVar('OVERRIDES') or '').split(':') else '' }"


FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Once lv_port_linux is fetched, initialise the lvgl submodule and
# move everything into lib/lv_port_linux in the usr working tree

python do_handle_fetch_git:prepend() {

    import os
    import subprocess

    ovrs = (d.getVar('OVERRIDES') or '').replace(' ', '').split(':')
    if 'lvgl' not in ovrs:
        return

    bb.plain("Fetching lv_port_linux submodule (lvgl)...")

    gitdir = os.path.join(d.getVar('WORKDIR'), 'git')

    subprocess.check_call(
        ['git', '-C', gitdir, 'submodule', 'update', '--init', '--recursive']
    )

    move_gitdir(d, 'lib/lv_port_linux')
}

do_install_apps:append () {

    if echo ":${OVERRIDES}:" | grep -q ":lvgl"; then
        usr_do_install_file_root "${IB_TARGET}/build/bin/fc-frontpage"
    fi
}

do_clean:append () {

    if echo ":${OVERRIDES}:" | grep -q ":lvgl"; then
        rm -rf ${IB_TARGET}/lib/lv_port_linux
        rm -rf ${WORKDIR}/git
    fi
}
