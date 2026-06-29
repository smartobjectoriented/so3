# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "LVGL Library for SO3"
DESCRIPTION = "LVGL SO3 with framebuffer"
LICENSE = "MIT"

# Fetch LVGL

SRCREV = "c033a98afddd65aaafeebea625382a94020fe4a7"
 
LVGL_URI = "git://github.com/lvgl/lvgl.git;branch=release/v9.3;protocol=https"

SRC_URI += " ${@ d.expand(d.getVar('LVGL_URI') or '') \
                 if 'lvgl' in (d.getVar('OVERRIDES') or '').split(':') else '' }"

# lvgl/slv/apps live entirely in the :lvgl override: the committed so3/usr is
# the lvgl-free base. These creation/CMake patches (applied to ${S} by do_patch,
# then mirrored to IB_TARGET by do_attach_infrabase) bring lib/slv, lib/lv_conf.h,
# the lvgl_*.elf apps and the lvgl/slv CMakeLists blocks back when :lvgl is set.
FILESEXTRAPATHS:prepend := "${THISDIR}/files/0001-${PF}:"
require files/0001-${PF}-patches.inc

SRC_URI += " ${@ d.expand(d.getVar('LVGL_PATCHES') or '') \
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

    # Copy lvgl into the workdir copy (${S}/lib/lvgl). do_attach_infrabase
    # (base.bbclass, before do_configure) then mirrors ${S} back into
    # IB_TARGET, so the fetched lvgl reaches the build alongside the
    # :lvgl patches applied to ${S} by do_patch.

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
    import os, shutil, subprocess
    if 'lvgl' not in (d.getVar('OVERRIDES') or '').split(':'):
        return

    target = d.getVar('IB_TARGET')
    s = d.getVar('S')
    ib_dir = d.getVar('IB_DIR')

    # Everything the :lvgl override creates in the committed-but-build-mutated
    # usr/ tree (lib/slv, lib/lvgl, lib/lv_conf.h, the lvgl_*.elf sources and
    # fillfb.c). do_attach_infrabase mirrors the patched ${S} back into
    # IB_TARGET, so these end up in BOTH; remove from both so the next :lvgl
    # build re-applies the creation patches onto a clean base instead of
    # failing with "already exists" (idempotent :lvgl rebuilds).
    created = [
        'lib/lvgl', 'lib/slv', 'lib/lv_conf.h', 'src/lib',
        'src/fillfb.c', 'src/lvgl_demo.c', 'src/lvgl_perf.c',
        'src/lvgl_benchmark.c', 'src/lvgl_widgets.c',
    ]
    for base in (target, s):
        if not base:
            continue
        for rel in created:
            p = os.path.join(base, rel)
            if os.path.isdir(p):
                shutil.rmtree(p, ignore_errors=True)
            elif os.path.exists(p):
                try:
                    os.remove(p)
                except OSError:
                    pass

    # The lvgl CMakeLists blocks are added by the :lvgl patches to the tracked
    # lib/CMakeLists.txt and src/CMakeLists.txt; restore them to the committed
    # lvgl-free base (git index) so the tree fully matches the base again.
    if ib_dir and os.path.isdir(os.path.join(ib_dir, '.git')):
        for rel in ('lib/CMakeLists.txt', 'src/CMakeLists.txt'):
            subprocess.run(['git', '-C', ib_dir, 'checkout', '--',
                            os.path.join(target, rel)], check=False)
}