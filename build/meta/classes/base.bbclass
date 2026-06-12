#
# Copyright OpenEmbedded Contributors
#
# SPDX-License-Identifier: MIT
#

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
 
do_attach_infrabase () {
	echo "Attaching ${PN} to ${IB_TARGET}"
	
	if [ -d "${IB_TARGET}" ]; then
		rm -rf ${IB_TARGET}.back
		mv ${IB_TARGET} ${IB_TARGET}.back
	fi
	
	mkdir -p ${IB_TARGET}
	cp -r ${S}/. ${IB_TARGET}
}

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


