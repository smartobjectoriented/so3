#
# Copyright OpenEmbedded Contributors
#
# SPDX-License-Identifier: MIT
#

inherit logging
inherit patch
inherit utils

# Specific to IB: we always consider the ${S} directory as our source patched directory.
# Hence, we have to move all specific subdirs like git to this subdir. 
# At the moment, only git is supported.

FILESPATH = "${@base_set_filespath(["${FILE_DIRNAME}/${P}", "${FILE_DIRNAME}/${PN}", "${FILE_DIRNAME}/files"], d)}"

# THISDIR only works properly with imediate expansion as it has to run
# in the context of the location its used (:=)

THISDIR = "${@os.path.dirname(d.getVar('FILE'))}"

python () {
    import sys
    import os

    # Fetch the BBLAYERS variable from the BitBake datastore
    bblayers = d.getVar('BBLAYERS', True)
    if bblayers:
        for layer_path in bblayers.split():
            lib_path = os.path.join(layer_path, 'lib')
            if os.path.isdir(lib_path) and lib_path not in sys.path:
                sys.path.insert(0, lib_path)

    need_machine = d.getVar('COMPATIBLE_PLATFORM')
    if need_machine and not bb.utils.to_boolean(d.getVar('PARSE_ALL_RECIPES', False)):
        import re
        compat_machines = (d.getVar('PLATFORMOVERRIDES') or "").split(":")
        for m in compat_machines:
            if re.match(need_machine, m):
                break
        else:
            raise bb.parse.SkipRecipe("incompatible with machine %s (not in COMPATIBLE_PLATFORM)" % d.getVar('IB_PLATFORM'))

}


BB_DEFAULT_TASK ?= "build"
CLASSOVERRIDE ?= "class-target"

die() {
	bbfatal_log "$*"
}

BASEDEPENDS = ""

DEPENDS:prepend = "${BASEDEPENDS} "
 
# THISDIR only works properly with imediate expansion as it has to run
# in the context of the location its used (:=)

THISDIR = "${@os.path.dirname(d.getVar('FILE'))}"

addtask fetch
do_fetch[dirs] = "${DL_DIR}"
do_fetch[file-checksums] = "${@bb.fetch.get_checksum_file_list(d)}"

do_fetch[vardeps] += "SRCREV"
python do_fetch() {

    src_uri = (d.getVar('SRC_URI') or "").split()
    if not src_uri:
        return

    try:
        fetcher = bb.fetch2.Fetch(src_uri, d)
        fetcher.download()
    except bb.fetch2.BBFetchException as e:
        bb.fatal("Bitbake Fetcher Error: " + repr(e))
}

addtask listtasks
do_listtasks[nostamp] = "1"
python do_listtasks() {
    taskdescs = {}
    maxlen = 0
    for e in d.keys():
        if d.getVarFlag(e, 'task'):
            maxlen = max(maxlen, len(e))
            if e.endswith('_setscene'):
                desc = "%s (setscene version)" % (d.getVarFlag(e[:-9], 'doc') or '')
            else:
                desc = d.getVarFlag(e, 'doc') or ''
            taskdescs[e] = desc

    tasks = sorted(taskdescs.keys())
    for taskname in tasks:
        bb.plain("%s  %s" % (taskname.ljust(maxlen), taskdescs[taskname]))
}

do_unpack[dirs] = "${WORKDIR}"
do_unpack[cleandirs] = "${@d.getVar('S') if os.path.normpath(d.getVar('S')) != os.path.normpath(d.getVar('WORKDIR')) else os.path.join('${S}', 'patches')}"

python do_unpack() {
    src_uri = (d.getVar('SRC_URI') or "").split()
    if not src_uri:
        return

    try:
        fetcher = bb.fetch2.Fetch(src_uri, d)
        fetcher.unpack(d.getVar('WORKDIR'))
    except bb.fetch2.BBFetchException as e:
        bb.fatal("Bitbake Fetcher Error: " + repr(e))
}

addtask do_handle_symlinks
python do_handle_symlinks() {
    import subprocess
    
    s = d.getVar('IB_SYMLINK:%s' % d.getVar('PN'))
    
    if s:
        l = d.getVar('IB_SYMLINK:%s' % d.getVar('PN')).split()
    
        for i in range(0, len(l), 3):
            where = l[i]
            src = l[i+1] if i + 1 < len(l) else None
            link = l[i+2] if i + 2 < len(l) else None
 
            where = d.getVar('S') + '/' + where 
 
            statement = 'ln -fs ' + src + ' ' + link
            subprocess.call(statement, shell=True, cwd=where)
}

def move_gitdir(d, dst_dir):
    import os
    import subprocess
    import shlex

    target_dir = d.getVar('S')
    gitdir = os.path.join(d.getVar('WORKDIR'), 'git')
    
    # Make sure the target directory exists
    cmd = f"mkdir -p {target_dir}/{dst_dir}"
    result = subprocess.run(cmd, shell=True, check=True)

    # Copy everything except .git, preserving symlinks/metadata
    cmd = (
        "find . -mindepth 1 -path './.git' -prune -o "
        "-exec cp -a --parents -t {} {{}} +"
    ).format(shlex.quote(os.path.join(target_dir, dst_dir)))
    
    result = subprocess.run(cmd, shell=True, check=True, cwd=gitdir)

    # Now erase the git directory
    cmd = f"rm -rf {gitdir}"
    result = subprocess.run(cmd, shell=True, check=True)


# If some files are fetched from a git directory, bitbake
# unpacked them to ${WORKDIR}/git directory. So, we want
# to move the contents to the target ${S} directory so that
# doing a updiff task will use the same approach for all recipes

# Additional note: this function can be overriden in recipes
# (.bbappend or other) using prepend (if many). So, the function
# may return.

python do_handle_fetch_git() {
    import os
    import subprocess
    import shlex
 
    workdir = d.getVar('WORKDIR')
    src_dir = os.path.join(workdir, 'git')

    if not os.path.isdir(src_dir):
        bb.note(f"Source directory {src_dir} does not exist — skipping copy.")
        return

    move_gitdir(d, '.')
 
}
do_unpack[postfuncs] = "do_handle_fetch_git"
 
# Default to the safe behaviour; set IB_FORCE_ATTACH=1 (env or local.conf)
# to override the dirty-tree guard below.
IB_FORCE_ATTACH ??= "0"

do_attach_infrabase () {
	ib_manifest="${IB_TARGET}.attach.sha256"

	# Guard against silently clobbering local edits.
	#
	# do_attach_infrabase regenerates ${IB_TARGET} from the freshly
	# fetched+patched ${S}. The fetched component trees (avz/, u-boot/,
	# qemu/, ...) are gitignored, so an edit made directly in ${IB_TARGET}
	# that hasn't been folded back into the patch set (via do_updiff) has no
	# version-control safety net — re-attaching would destroy it. After each
	# attach we record a sha256 manifest of the files we wrote; on the next
	# attach we verify those files are still untouched and abort if any were
	# modified or removed. Build artefacts produced by a later `make` are not
	# in the manifest, so a freshly-built tree still verifies clean. Newly
	# *added* source files are not tracked by the manifest — the ${IB_TARGET}.back
	# copy remains the last-resort net for that case.
	if [ -d "${IB_TARGET}" ] && [ -f "$ib_manifest" ] && [ "${IB_FORCE_ATTACH}" != "1" ]; then
		# The quilt staging left by do_patch (patches/, series, .pc) is a
		# build product, not source -- it is regenerated on every patch run
		# and is gitignored. Recorded in the manifest it made the guard fire
		# on its own artefacts and refuse a perfectly clean tree. Filtered
		# here as well as pruned below, so manifests recorded before this
		# fix stop blocking too. `|| true`: grep exits 1 when it filters
		# everything away, and bitbake runs shell tasks under `set -e`.
		ib_dirty=$(cd "${IB_TARGET}" && LC_ALL=C sha256sum -c --quiet "$ib_manifest" 2>/dev/null \
			| sed -n 's/: FAILED.*$//p' \
			| grep -vE '(^|/)(\.pc|patches)/' || true)
		if [ -n "$ib_dirty" ]; then
			bbwarn "Local modifications detected in ${IB_TARGET}, not captured in the ${PN} patch set:"
			echo "$ib_dirty" | sed 's|^\./|    |' >&2
			bbfatal "Refusing to re-attach ${PN} (would overwrite the files above). Run 'bitbake ${PN} -c updiff' to fold them into the patch set, or set IB_FORCE_ATTACH=1 to discard them and re-attach (prior tree is kept in ${IB_TARGET}.back)."
		fi
	fi

	echo "Attaching ${PN} to ${IB_TARGET}"

	if [ -d "${IB_TARGET}" ]; then
		rm -rf ${IB_TARGET}.back
		mv ${IB_TARGET} ${IB_TARGET}.back
	fi

	mkdir -p ${IB_TARGET}
	cp -r ${S}/. ${IB_TARGET}

	# Record what we just wrote so the next attach can detect local edits.
	( cd "${IB_TARGET}" && find . \( -name .pc -o -name patches \) -prune -o -type f -print0 \
		| LC_ALL=C sort -z | xargs -0 sha256sum ) > "$ib_manifest" 2>/dev/null || rm -f "$ib_manifest"

	# Record WHICH recipe the tree now belongs to. Several recipes can share
	# one IB_TARGET -- uboot_2022.04 and uboot_2024.07 both attach to
	# u-boot/ -- and each has its own do_attach_infrabase stamp. Without
	# this marker, attaching one and then building the other finds a valid
	# stamp, skips the attach, and compiles the wrong source tree. See
	# do_check_attach() in this class.
	echo "${PF}" > "${IB_TARGET}.attach.pf"
}

# Refuse to build against a tree that belongs to another recipe.
#
# IB_TARGET is a path in the source tree, not under tmp/, and more than one
# recipe can claim it: uboot_2022.04 and uboot_2024.07 both attach to
# u-boot/, selected by PREFERRED_VERSION per platform. do_attach_infrabase
# is stamped per recipe, so after building platform A and switching to
# platform B, B's recipe finds its own stamp from an earlier session, skips
# the attach, and runs do_configure against A's sources.
#
# That fails loudly when the defconfig is missing -- rpi4_64_defconfig in a
# 2024.07 tree -- and SILENTLY when it is not. A virt64 image was built with
# U-Boot 2022.04 instead of 2024.07; the two differ in
# CONFIG_POSITION_INDEPENDENT, so ATF loaded BL33 at 0x60000000 and jumped
# into a binary linked for 0x0. No output, no error, and a whole secure boot
# chain to bisect before the cause turned out to be the build system.
#
# The marker written by do_attach_infrabase names the recipe that owns the
# tree. This task is nostamp, so it is the one thing that always runs, and it
# refuses rather than repairing: re-attaching means deleting a tree the user
# may have edited, and that decision is theirs. A tree with no marker was
# attached before markers existed and is adopted silently.

do_check_attach[nostamp] = "1"
python do_check_attach() {
    import os

    target = d.getVar('IB_TARGET')
    pf = d.getVar('PF')
    if not target or not pf or not os.path.isdir(target):
        return

    try:
        with open(target + '.attach.pf') as f:
            owner = f.read().strip()
    except OSError:
        return

    if owner == pf:
        return

    bb.fatal(
        "%s is attached to %s, not to %s.\n"
        "Both recipes share this directory, and each has its own attach "
        "stamp, so building now would compile the other recipe's sources.\n"
        "Re-attach before continuing:\n"
        "    rm -f %s.do_attach_infrabase\n"
        "    IB_FORCE_ATTACH=1 bitbake %s -c attach_infrabase\n"
        "Anything you edited in %s that is not in the patch series will be "
        "lost; the previous tree is kept as %s.back."
        % (target, owner, pf, d.getVar('STAMP'), d.getVar('PN'), target, target)
    )
}

addtask do_check_attach after do_attach_infrabase before do_configure

addtask cleansstate after do_clean
python do_cleansstate() {
        sstate_clean_cachefiles(d)
}
addtask cleanall after do_cleansstate
do_cleansstate[nostamp] = "1"

python do_cleanall() {
    src_uri = (d.getVar('SRC_URI') or "").split()
    if not src_uri:
        return

    try:
        fetcher = bb.fetch2.Fetch(src_uri, d)
        fetcher.clean()
    except bb.fetch2.BBFetchException as e:
        bb.fatal(str(e))
}
do_cleanall[nostamp] = "1"

addtask do_fetch before do_unpack
addtask do_unpack before do_patch
addtask do_patch before do_handle_symlinks
addtask do_handle_symlinks before do_attach_infrabase
addtask do_attach_infrabase before do_configure
addtask do_configure before do_build
addtask do_build 

EXPORT_FUNCTIONS do_fetch listtasks do_unpack do_attach_infrabase do_handle_symlinks


