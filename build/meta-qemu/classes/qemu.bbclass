
# Class for building QEMU in infrabase

qemu_do_configure () {

	echo "Configuring QEMU..."
	cd ${IB_DIR}/qemu
	./configure ${QEMU_CONFIGURE:${IB_PLATFORM}}

}

do_build[nostamp] = "1"
do_build () {
	echo "Building QEMU with ${CORES} cores..."
	cd ${IB_DIR}/qemu
	make -j${CORES}
}

EXPORT_FUNCTIONS do_configure

