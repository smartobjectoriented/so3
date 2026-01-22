#!/bin/sh

set -e

usage() {
  echo "$0 [OPTIONS]"
  echo "  -c        Clean"
  echo "  -r        Release build"
  echo "  -v        Verbose"
  echo "  -s        Single core"
  echo "  -h        Print this help"
}

# Place the debug info of the applications in a separate file
strip_debug_info()
{
  # retrieve the correct objcop tool
  if [ "$1" = "virt32" ] || [ "$1" = "rpi4" ]; then
    OBJCOPY="arm-linux-musleabihf-objcopy"
  else
    OBJCOPY="aarch64-linux-musl-objcopy"
  fi

  for app in build/deploy/*.elf; do
    [ -e "$app" ] || continue

    # 1. Create a file with only '.debug_*' sections
    $OBJCOPY --only-keep-debug $app $app.debug

    # 2. remove/strip debug info
    $OBJCOPY --strip-all $app

    # 3. Connect the two files
    $OBJCOPY --add-gnu-debuglink=$app.debug $app

  done
}


install_file_elf() {
  if [ -f $1 ] ; then 
    for subfolder_app in $(find build/src -type f -iname "*.elf"); do
      mv "$subfolder_app" build/deploy
    done
  fi
}

install_file_root() {
  [ -f $1 ] && echo "Installing $1" && cp $1 build/deploy
}

install_directory_root() {
  [ -d $1 ] && cp -R $1 build/deploy
}

install_file_directory() {
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

while getopts crhvs option
  do
    case "${option}"
      in
        c) clean=y;;
        r) debug=n;;
        v) verbose=y;;
        s) singlecore=y;;
        h) usage && exit 1;;
    esac
  done

ORIGINAL_WD=$(pwd)
SCRIPT=$(readlink -f $0)
SCRIPTPATH=`dirname $SCRIPT`

if [ $clean = y ]; then
  echo "Cleaning $SCRIPTPATH/build"
  rm -rf $SCRIPTPATH/build
  if [ "$PLATFORM" = "virt64" ]; then
    echo "Cleaning microPython"
    cd src/micropython/ports/soo
    make clean
    cd -
  fi
  exit
fi

if [ $debug = y ]; then
  build_type="Debug"
else
  build_type="Release"
fi

echo "Starting $build_type build"
mkdir -p $SCRIPTPATH/build

cd $SCRIPTPATH/build

if [ "$PLATFORM" = "virt32" -o "$PLATFORM" = "vexpress" -o "$PLATFORM" = "rpi4" ]; then
  default_toolchain="arm-linux-musl.cmake"
elif [ "$PLATFORM" = "virt64" -o "$PLATFORM" = "rpi4_64" ]; then
  default_toolchain="aarch64-linux-musl.cmake"
elif [ -z "$USR_BUILD_TOOLCHAIN_FILE" ]; then
  # Only fail if no custom toolchain is provided
  echo "Unsupported PLATFORM ($PLATFORM) and no TOOLCHAIN_FILE specified"
  exit 1
fi

# Use custom toolchain if TOOLCHAIN_FILE is set, otherwise use the default
toolchain_file="${USR_BUILD_TOOLCHAIN_FILE:-$default_toolchain}"

# Run cmake with the selected toolchain
cmake -Wno-dev --no-warn-unused-cli -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_TOOLCHAIN_FILE=../"$toolchain_file" ..

if [ $singlecore = y ]; then
  NRPROC=1
else
  NRPROC=$(nproc)
fi

if [ $verbose = y ]; then
	make VERBOSE=1 -j1
else
	make -j$NRPROC
fi

cd $ORIGINAL_WD

mkdir -p build/deploy/

install_directory_root out

install_file_elf

strip_debug_info "$PLATFORM"


exit 0
