obj-y 			+= cpuid.o
obj-y 			+= cpu.o
obj-y 			+= cpu_init.o
obj-y			+= fsl_law.o
ifeq ($(CONFIG_E500MC),y)
obj-y			+= speed_e500mc.o liodn.o portals.o t1024_ids.o
#obj-y			+= speed_e500mc.o
else
obj-y			+= speed.o
endif
obj-y			+=time.o
obj-y			+= fsl_gpio.o
obj-y			+= fsl_i2c.o
obj-y			+= fsl_usb.o
obj-$(CONFIG_MP)	+= mp.o
obj-$(CONFIG_OFTREE)	+= fdt.o
obj-$(CONFIG_DRIVER_NET_GIANFAR) += eth-devices.o
obj-$(CONFIG_FSL_DDR2) += ../ddr-8xxx/
obj-$(CONFIG_FSL_DDR3) += ../ddr-8xxx/
extra-y += barebox.lds
