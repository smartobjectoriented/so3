
inherit filesystem
inherit logging
inherit rootfs

# Class for managing the user space environment

# Deployment of user space components

usr_do_install_file_root () {
	echo "Installing $1"
	
	mkdir -p ${IB_TARGET}/build/deploy/root
	cp $1 ${IB_TARGET}/build/deploy/root
}

usr_do_install_directory_root () {
	echo "Installing $1"
	
	mkdir -p ${IB_TARGET}/build/deploy/root
	cp -R "$1" "${IB_TARGET}/build/deploy/root"
}

usr_do_install_file_dir () {
	echo "Installing $1 into $2" 
	mkdir -p ${IB_TARGET}/build/deploy/$2 
	cp -r $1 ${IB_TARGET}/build/deploy/$2
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
do_clean () {

	# Remove all patches
	rm -rf ${IB_TARGET}/patches
	
	# Clean the user space apps
	rm -rf ${IB_TARGET}/build
}




