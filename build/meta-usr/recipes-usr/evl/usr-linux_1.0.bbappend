
SUMMARY = "Add-ons for Linux EVL realtime applications"
DESCRIPTION = "Hard realtime applications using EVL/Xenomai 4 extension"
LICENSE = "GPLv2"

EVL_URI = "git://gitlab.com/Xenomai/xenomai4/libevl.git;branch=master;tag=r49;protocol=https;name=evl"
SRCREV_evl = "8d9d73b4e074d0762930a2ae94125ab92e19e57f"

SRC_URI += " ${@ d.expand(d.getVar('EVL_URI') or '') \
                 if 'evl' in (d.getVar('OVERRIDES') or '').split(':') else '' }"
 
# These patches bring evl usr apps
FILESPATH:prepend = "${THISDIR}/../evl/files/0001-${PF}:"

require files/0001-${PF}-patches.inc

python do_handle_fetch_git:prepend() {

    import os
    import subprocess
    import shlex
  
    # We compare against the value we expect.
    # If it is not equal, we give a chance to other (same) functions
    # in .bbappend to be executed.

    ovrs = (d.getVar('OVERRIDES') or '').replace(' ', '').split(':')
    if 'evl' in ovrs:
        
        bb.plain("Now, setting up libevl in usr ...")

        move_gitdir(d, 'lib/evl')
}

do_configure:prepend() {

  cd ${IB_TARGET}/lib/evl
	 
	mkdir -p build
	cd build
	
  meson setup --cross-file ~/edgemtech/customer/cybelec_linux/cybelec_linux/ib/linux/usr/lib/evl/meson/aarch64-none-linux-gnu -Dbuildtype=release -Dprefix=/usr/evl -Duapi=${IB_LINUX_PATH} . ${IB_TARGET}/lib/evl
}

do_build:prepend() {
  
  cd ${IB_TARGET}/lib/evl/build
  meson compile

}

# Installing usr apps mean to move the binary and all files which need to
# be copied to the rootfs. Be aware that it is a deploy directory and not
# the rootfs itself; this is achieved with the do_deploy task (by the bsp recipe)

do_install_apps:append () {
   
    if echo ":${OVERRIDES}:" | grep -q ":evl"; then

      usr_do_install_file_root "${IB_TARGET}/build/src/evl"
 
    fi
}

do_clean:append() {
  if echo ":${OVERRIDES}:" | grep -q ":evl"; then

    rm -rf ${IB_TARGET}/lib/evl
    
    rm -rf ${S}
    
    [ -f ${IB_TARGET}.back/src/CMakeLists.txt ] && \
      cp ${IB_TARGET}.back/src/CMakeLists.txt ${IB_TARGET}/src/ || true
  
   [ -f ${IB_TARGET}.back/src/modules/modtry.c ] && \
      cp ${IB_TARGET}.back/src/modules/modtry.c ${IB_TARGET}/src/modules/ || true

  fi
}
