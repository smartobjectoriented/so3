
# Class of Linux layer

inherit logging

IB_LINUX_PATH = "${IB_DIR}/linux/linux"


def __do_clean(d):

    import os
    from bb.process import run

    tmpdir = d.getVar('TMPDIR')
    ib_target = d.getVar('IB_TARGET')

    # remove stamps
    run(f"rm -f {tmpdir}/stamps/linux*")

    # run make distclean if .config exists
    if ib_target and os.path.isdir(ib_target):
        if os.path.exists(os.path.join(ib_target, ".config")):
            run("make distclean", cwd=ib_target)
