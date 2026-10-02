# Copyright (c) 2026 EDGEMTech SA

# `deploy.sh linux`: put the kernel from the last `build.sh linux` on the
# target. A kernel is never copied on its own — it travels in the ITB, which
# bsp-linux packs and writes onto p1 (bsp-linux:do_deploy_boot). No
# dependency on do_build: deploy writes what the build produced, it does not
# compile.
#
# Not in linux.bbclass: bsp-linux inherits that class and has its own
# do_deploy.

do_deploy[nostamp] = "1"
do_deploy[depends] = "bsp-linux:do_deploy_boot"
python do_deploy () {
    bb.plain("Linux kernel deployed (ITB on p1)")
}
addtask do_deploy
