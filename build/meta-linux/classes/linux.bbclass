# Copyright (c) 2025-2026 EDGEMTech SA

# Class of Linux layer


IB_LINUX_PATH = "${IB_DIR}/linux/linux"

# The kernel is built in place, and kbuild leaves generated sources next to the
# real ones that patch.bbclass's generic excludes do not cover; without these,
# updiff turns a plain `build.sh linux` into two dozen bogus patches. All are
# absent from the upstream tarball (drivers/s390/char/defkeymap.c aside, which
# matches by basename only and is irrelevant to the ARM targets of this tree).
IB_UPDIFF_EXCLUDE ??= "dtbs-list vdso.lds *.asn1.c *.asn1.h *.dtb.S \
    scsi_devinfo_tbl.c consolemap_deftbl.c defkeymap.c logo_*.c \
    utsversion-tmp.h bounds.s crc32table.h crc64table.h oid_registry_data.c \
    vdso32.lds hyp.lds hyp-reloc.S hyp-constants.s hyp_constants.h \
    sha256-core.S sha512-core.S rq-offsets.s \
    ucs_fallback_table.h ucs_recompose_table.h ucs_width_table.h \
    *.gen.S shipped-certs.c"


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
