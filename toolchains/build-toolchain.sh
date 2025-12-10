#!/bin/bash
#
# Copyright (C) 2025 Jean-Pierre Miceli <jean-pierre.miceli@heig-vd.ch>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

# This script build 'arm' & 'aarch64' MUSL toolchains

SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

# output PATH - select where the toolchain will be installed
#
# The final path will be:
#    * arm:     <OUTPUT_PATH>/arm-linux-musleabihf
#    * aarch64: <OUTPUT_PATH>/aarch64-linux-musl
#    default: '<CURRENT DIR>/aarch64-linux-musl' and '<CURRENT DIR>/arm-linux-musleabihf'
#OUTPUT_PATH=

GIT_COMMIT="3635262"

AARCH64_PATH='aarch64-linux-musl'
ARM_PATH='arm-linux-musleabihf'

if [[ $EUID -ne 0 ]]; then
    echo "Please run as root"
    exit 1
fi

pushd $SCRIPTPATH

if [[ -v $OUTPUT_PATH ]]; then
    OUTPUT=$OUTPUT_PATH
else
    OUTPUT=$SCRIPTPATH
fi

echo "== base installation path is '$SCRIPTPATH'"

# 1. Retrieve the repo
git clone https://github.com/richfelker/musl-cross-make
cd musl-cross-make
git checkout $GIT_COMMIT

# Compile & install 'aarch64-linux-musl'
echo "== Compiling 'aarch64-linux-musl' (installation path: $OUTPUT/$AARCH64_PATH"
cp ../config.mak.aarch64 config.mak
echo "OUTPUT = $OUTPUT/$AARCH64_PATH" >> config.mak
make && sudo make install

# Compile & install 'arm-linux-musleabihf'
echo "== Compiling 'arm-linux-musleabihf'"
make clean
cp ../config.mak.arm config.mak
echo "OUTPUT = $OUTPUT/$ARM_PATH" >> config.mak
make && sudo make install

popd
