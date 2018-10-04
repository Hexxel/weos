# DRAM training file binary.0

## Build

To build the binary.0 file clone the following repository:

github.com:westermo/u-boot-marvell.git

and change to branch:

wmo/v2018.10.0

then do a:

make dagger

which should produce a binary.0 in the westermo directory. Copy the
file to barebox/westermo directory and build barebox as usual. The
binary.0 should be integrated in the dagger-pbl-spi.bin file.
