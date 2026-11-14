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

    # The tree holds the lvgl series if it was attached with it, whatever the
    # overrides say now. Any other tree is left alone: a clean keeps local edits.

    series = {os.path.basename(u) for u in (d.getVar('LVGL_PATCHES') or '').split()}
    if 'lvgl' not in (d.getVar('OVERRIDES') or '').split(':') \
            and not series & (ib_attached_series(d) or set()):
        return

    target = d.getVar('IB_TARGET')
    s = d.getVar('S')
    ib_dir = d.getVar('IB_DIR')

    # Everything the :lvgl override creates or replaces. do_attach_infrabase
    # mirrors the patched ${S} back into IB_TARGET, so these end up in BOTH.

    created = [
        'lib/lvgl', 'lib/slv', 'lib/lv_conf.h', 'src/lib',
        'src/fillfb.c', 'src/lvgl_demo.c', 'src/lvgl_perf.c',
        'src/lvgl_benchmark.c', 'src/lvgl_widgets.c',
    ]
    cmakelists = ['lib/CMakeLists.txt', 'src/CMakeLists.txt']

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

    # usr/ belongs to the so3 recipe, so its ${S} is the lvgl-free base, some of
    # these files included. Without a fetched so3, usr/ is under git.

    usr = None
    try:
        with open(os.path.dirname(target) + '.attach.pf') as f:
            pf = f.read().strip()
        usr = os.path.join(d.getVar('TMPDIR'), 'work', pf, pf.rsplit('-', 1)[0], 'usr')
    except OSError:
        pass

    if usr and os.path.isdir(usr):
        for rel in created + cmakelists:
            src = os.path.join(usr, rel)
            dst = os.path.join(target, rel)
            if os.path.isdir(src):
                shutil.copytree(src, dst, symlinks=True)
            elif os.path.isfile(src):
                shutil.copyfile(src, dst)
    elif ib_dir and os.path.isdir(os.path.join(ib_dir, '.git')):
        for rel in cmakelists:
            subprocess.run(['git', '-C', ib_dir, 'checkout', '--',
                            os.path.join(target, rel)], check=False)

    try:
        os.remove(target + '.attach.series')
    except OSError:
        pass
}
