#!/bin/bash
export ARCH=arm
export CROSS_COMPILE=arm-unknown-linux-gnueabi-
export GTAGS_MACH=arch/arm/mach-imx
make basis_defconfig
make
