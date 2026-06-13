
SUMMARY = "Add-ons for SOO user space environment"
DESCRIPTION = "Additional applications are used to manage SO3 capsules"
LICENSE = "GPLv2"

SRC_URI += " ${@ d.expand(d.getVar('SOO_URI') or '') \
                 if 'soo' in (d.getVar('OVERRIDES') or '').split(':') else '' }"
 
# These patches bring soo usr apps
FILESPATH:prepend = "${THISDIR}/../soo/files/0001-${PF}:"

require files/0001-${PF}-patches.inc
 
# Installing usr apps mean to move the binary and all files which need to
# be copied to the rootfs. Be aware that it is a deploy directory and not
# the rootfs itself; this is achieved with the do_deploy task (by the bsp recipe)

do_install_apps:append () {
   
    if echo ":${OVERRIDES}:" | grep -q ":soo"; then
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/injector"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/restoreme"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/saveme"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/melist"
        usr_do_install_file_root "${IB_TARGET}/build/src/soo/shutdownme"

        usr_do_install_file_root "${IB_TARGET}/build/src/soo/emiso_engine/emiso_engine"
    fi
}

do_clean:append() {
  if echo ":${OVERRIDES}:" | grep -q ":soo"; then

    rm -rf ${IB_TARGET}/src/soo   
    rm -rf ${IB_TARGET}/include/soo
    rm -rf ${IB_TARGET}/include/core

    rm -rf ${S}
    
    [ -f ${IB_TARGET}.back/src/CMakeLists.txt ] && \
      cp ${IB_TARGET}.back/src/CMakeLists.txt ${IB_TARGET}/src/ || true
  
  fi
}
