#!/bin/bash
export ARCH=ppc
export CROSS_COMPILE=powerpc-unknown-linux-gnu-
export GTAGS_MACH=arch/ppc/mach-mpc85xx
make corazon_defconfig
make
