#!/bin/bash
export ARCH=arm
export CROSS_COMPILE=arm-unknown-linux-gnueabi-
export GTAGS_MACH=arch/arm/mach-mvebu
make dagger_defconfig
make
