#!/bin/bash
# This little script creates a BareBox release
#
# To build 2016.05.0-1 call this script as:
#
#   ./bin/release.sh 2016.05.0-1
#
BASIS_TOOL=arm-unknown-linux-gnueabi-
CORAZON_TOOL=powerpc-unknown-linux-gnu-
CORONET_TOOL=powerpc-unknown-linux-gnu-
DAGGER_TOOL=arm-unknown-linux-gnueabi-

if [ $# != 1 ]; then
        echo "Usage: ./bin/release.sh <VERSION>"
        exit 1;
fi

if [ `git describe` != "v$1" ]; then
        echo "Not building from tag v$1";
        exit 1;
fi

set -e


unset LD_LIBRARY_PATH

echo "Building Basis with ${BASIS_TOOL}"
make ARCH=arm CROSS_COMPILE=${BASIS_TOOL} distclean
make ARCH=arm CROSS_COMPILE=${BASIS_TOOL} basis_defconfig
KERNELVERSION=$1 make ARCH=arm CROSS_COMPILE=${BASIS_TOOL}
mv barebox.bin barebox-basis-$1.bin
make ARCH=arm CROSS_COMPILE=${BASIS_TOOL} distclean

echo "Building Corazon with ${CORAZON_TOOL}"
make ARCH=ppc CROSS_COMPILE=${CORAZON_TOOL} distclean
make ARCH=ppc CROSS_COMPILE=${CORAZON_TOOL} corazon_defconfig
KERNELVERSION=$1 make ARCH=ppc CROSS_COMPILE=${CORAZON_TOOL}
mv barebox.bin barebox-corazon-$1.bin
make ARCH=ppc CROSS_COMPILE=${CORAZON_TOOL} distclean

echo "Building Coronet with ${CORONET_TOOL}"
make ARCH=ppc CROSS_COMPILE=${CORONET_TOOL} distclean
make ARCH=ppc CROSS_COMPILE=${CORONET_TOOL} coronet_defconfig
KERNELVERSION=$1 make ARCH=ppc CROSS_COMPILE=${CORONET_TOOL}
mv barebox.bin barebox-coronet-$1.bin
make ARCH=ppc CROSS_COMPILE=${CORONET_TOOL} distclean

echo "Building dagger with ${DAGGER_TOOL}"
make ARCH=arm CROSS_COMPILE=${DAGGER_TOOL} distclean
make ARCH=arm CROSS_COMPILE=${DAGGER_TOOL} dagger_defconfig
KERNELVERSION=$1 make ARCH=arm CROSS_COMPILE=${DAGGER_TOOL}
mv barebox.bin barebox-dagger-$1.bin
make ARCH=arm CROSS_COMPILE=${DAGGER_TOOL} distclean

