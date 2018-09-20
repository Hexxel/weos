obj-$(CONFIG_LIBWESTERMO) += main.o
obj-$(CONFIG_WMO_BACKPLANE) += backplane.o

CFLAGS_wmocp.o  = -I$(srctree)/fs/squashfs
CFLAGS_wmosum.o = -I$(srctree)/fs/squashfs
obj-$(CONFIG_LIBWESTERMO) += wmocp.o wmosum.o wmover.o product.o
