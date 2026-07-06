# Copyright (c) 2025-2026 EDGEMTech SA

inherit filesystem
inherit logging
inherit rootfs

# Class for managing the user space environment

# Deployment of user space components

usr_do_install_file_root () {
	echo "Installing $1"
	
	mkdir -p ${IB_TARGET}/build/deploy/root
	cp $1 ${IB_TARGET}/build/deploy/root 2>/dev/null || true
}

usr_do_install_directory_root () {
	echo "Installing $1"
	
	mkdir -p ${IB_TARGET}/build/deploy/root
	cp -R "$1" "${IB_TARGET}/build/deploy/root" 2>/dev/null || true
}

usr_do_install_file_dir () {
	echo "Installing $1 into $2" 
	mkdir -p ${IB_TARGET}/build/deploy/$2 
	cp -r $1 ${IB_TARGET}/build/deploy/$2 2>/dev/null || true
}


# The unpack task will erase ${S} before unpacking in it
# The copy of usr must be done after unpack

# Complete the working directory with our usr/ contents
do_unpack[postfuncs] = "retrieve_usr_dir"
do_unpack[postfuncs] += "do_handle_fetch_git"

def __retrieve_usr_dir(d):
    import os
    import subprocess
   
    bb.plain("-- Retrieve the usr dir into the workdir --")

    src_dir = d.getVar('IB_TARGET')
    dst_dir = d.getVar('S')

    # Copy the while contents of usr in the temporary working directory
    # so that it will be possible to handle the fetch of submodules before
    # the execution of attach_infrabase task

    cmd = f"find . -not -path '*/.git/*' -and -not -path '*/patches/*' -and \( -type f -or -type d -empty \) -exec cp -r --parents -t {dst_dir} {{}} +"
    result = subprocess.run(cmd, shell=True, check=True, cwd=src_dir)

python retrieve_usr_dir() {
    __retrieve_usr_dir(d)
}

# De-duplicate add_subdirectory() lines in CMakeLists.txt files.
# Implemented as a separate Python function and attached to do_configure via
# [prefuncs] — using `python do_configure:prepend()` directly mixes badly
# with the shell do_configure on some bitbake versions and the parser
# treats the python body as shell code.

python usr_dedup_add_subdirectory() {
    import os
    import glob

    target = d.getVar('IB_TARGET')
    for cmake_file in glob.glob(os.path.join(target, '**', 'CMakeLists.txt'), recursive=True):
        with open(cmake_file, 'r') as f:
            lines = f.readlines()

        seen = set()
        deduped = []
        removed = []
        for line in lines:
            key = line.strip()
            if key.startswith('add_subdirectory(') and key in seen:
                removed.append(key)
            else:
                if key.startswith('add_subdirectory('):
                    seen.add(key)
                deduped.append(line)

        if removed:
            bb.warn("Duplicate add_subdirectory removed in %s: %s" % (cmake_file, ', '.join(removed)))
            with open(cmake_file, 'w') as f:
                f.writelines(deduped)
}

do_configure[prefuncs] += "usr_dedup_add_subdirectory"

do_configure () {
    mkdir -p ${IB_TARGET}/build
}

# Build of user space custom applications

do_build[nostamp] = "1"
do_build () {

	mkdir -p ${IB_TARGET}/build
	cd ${IB_TARGET}/build
	 
	# User space applications
	cmake -Wno-dev --no-warn-unused-cli -DCMAKE_BUILD_TYPE=${IB_USR_BUILD_TYPE} \
		-DCMAKE_KERNEL_PATH=${IB_LINUX_PATH} -DCMAKE_TOOLCHAIN_FILE=${IB_TOOLCHAIN_PATH} ..
	 
	make -j${CORES}

	cd ${IB_TARGET}

	# And now the (local) deployment
	mkdir -p ${IB_TARGET}/build/deploy
		
	# Proceed with local deployment
	do_install_apps
	
}

do_clean[nostamp] = "1"
addtask do_clean
# Implemented in Python on purpose: a *shell* task needs ${WORKDIR}/temp to
# exist so bitbake can create its output fifo there, but do_clean often runs
# when that dir is absent (fresh tree / already-cleaned), giving
# "No such file or directory: .../temp/fifo.NNNN". Python tasks create the
# temp dir themselves and use no fifo, so they are robust here.
python do_clean() {
    import os, shutil
    target = d.getVar('IB_TARGET')
    # Remove the applied patches and the (in-tree) user-space build dir.
    shutil.rmtree(target + '/patches', ignore_errors=True)
    shutil.rmtree(target + '/build', ignore_errors=True)

    # Clear the do_attach_infrabase manifest (${IB_TARGET}.attach.sha256) so the
    # next attach re-attaches from a fresh fetch+patch instead of aborting with
    # "Refusing to re-attach". Generic to every usr recipe: the usr build mutates
    # IB_TARGET in place (e.g. the :lvgl override fetches lib/lvgl and renames its
    # stray CMakeLists.txt to hide them from the recursive CMake glob), and those
    # renames/removals make the manifest's `sha256sum -c` fail on the next attach.
    # A clean is an explicit reset, and the attach still backs the tree up to
    # ${IB_TARGET}.back, so dropping the manifest here is the safe, generic fix —
    # the guard stays fully active for builds that were NOT preceded by a clean.
    try:
        os.remove(target + '.attach.sha256')
    except OSError:
        pass
}




