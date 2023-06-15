#!/bin/bash

function usage {
  echo "$0 [OPTIONS]"
  echo "  -c        Clean"
  echo "  -d        Debug build"
  echo "  -v        Verbose"
  echo "  -s        Single core"
  echo "  -h        Print this help"
}

function install_file_elf {
  [ -f $1 ] && echo "Installing $1" && mv build/src/*.elf build/deploy && mv build/src/**/*.elf build/deploy
}

function install_file_root {
  [ -f $1 ] && echo "Installing $1" && cp $1 build/deploy
}

function install_directory_root {
  [ -d $1 ] && echo "Installing $1" && cp -R $1 build/deploy
}

function install_file_directory {
  [ -f $1 ] && echo "Installing $1 into $2" && mkdir -p build/deploy/$2 && cp $1 build/deploy/$2
}


while read var; do
if [ "$var" != "" ]; then
  export $(echo $var | sed -e 's/ //g' -e /^$/d -e 's/://g' -e /^#/d)
fi
done < ../build.conf

echo Platform is ${PLATFORM}

clean=n
debug=y
verbose=n
singlecore=n

while getopts cdhvs option
  do
    case "${option}"
      in
        c) clean=y;;
        d) debug=y;;
	    v) verbose=y;;
        s) singlecore=y;;
        h) usage && exit 1;;
    esac
  done

SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`

if [ $clean == y ]; then
  echo "Cleaning $SCRIPTPATH/build"
  rm -rf $SCRIPTPATH/build
  exit
fi

if [ $debug == y ]; then
  build_type="Debug"
else
  build_type="Release"
fi

echo "Starting $build_type build"
mkdir -p $SCRIPTPATH/build

cd $SCRIPTPATH/build

if [ "$PLATFORM" == "virt32" -o "$PLATFORM" == "vexpress" -o "$PLATFORM" == "rpi4" ]; then
cmake -Wno-dev --no-warn-unused-cli -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_TOOLCHAIN_FILE=../arm_toolchain.cmake ..
fi
if [ "$PLATFORM" == "virt32" -o "$PLATFORM" == "virt64" -o "$PLATFORM" == "rpi4_64" ]; then
cmake -Wno-dev --no-warn-unused-cli -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_TOOLCHAIN_FILE=../aarch64_toolchain.cmake ..
fi
if [ $singlecore == y ]; then
    NRPROC=1
else
    NRPROC=$((`cat /proc/cpuinfo | awk '/^processor/{print $3}' | wc -l` + 1))
fi
if [ $verbose == y ]; then
	make VERBOSE=1 -j1
else
	make -j$NRPROC
fi
cd -


mkdir -p build/deploy/

# SO3 shell
install_directory_root usr/out

install_file_elf





