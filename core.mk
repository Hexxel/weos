############################################################################
#
# Get the core stuff worked out
#
.EXPORT_ALL_VARIABLES:

SCAN_OUTPUT        ?= /tmp/

REQ_TOOL_VER        = crosstool-ng-1.22.0-126-gd7339f5
TOOL_VERSION        = $(shell $(ROOTDIR)/bin/toolchain-version.sh $(CROSS_COMPILE)gcc)

VERFILE             = $(ROOTDIR)/westermo/include/otn/westermo.h
LINUXDIR            = kernel/linux-build
DEBARCHDIR          = $(DEBDIR)/upgrade/$(CONFIG_FAMILY)
DOWNLOADS	   ?= $(shell xdg-user-dir DOWNLOAD  2>/dev/null || echo "$(ROOTDIR)/downloads")
PERSISTENT_STORE   ?= $(shell xdg-user-dir DOCUMENTS 2>/dev/null || echo "$(IMAGEDIR)")
CONFIG_MTD_DEFAULT ?= $(PERSISTENT_STORE)/Config.mtd
BIN_REPO	   ?= ftp://ftp.labs.westermo.se/dist
REL_REPO           ?= ftp://ftp.labs.westermo.se/releases
FINIT_PLUGIN_PATH    = /lib/finit/plugins
LINUX_MAJOR_VERSION  = $(shell echo $(CONFIG_LINUX_VERSION)   | sed 's/\([0-9]*\.[0-9]*\).*/\1/')
TEMPFILE            := $(shell mktemp)

ifdef CONFIG_FAMILY
PROD_TUPLE           = $(CONFIG_FAMILY)/$(CONFIG_PRODUCT)
endif
SYS_ROOT             = $(dir $(shell $(CROSS)gcc -print-sysroot)/)
LINUX_CONFIG         = $(ROOTDIR)/$(LINUXDIR)/.config
KERNEL               = $(ROOTDIR)/$(LINUXDIR)/arch/$(SYSTEM_ARCH)/boot/$(LINUXTARGET)
CONFIG_CONFIG        = $(ROOTDIR)/config/.config
MODULES_CONFIG       = $(ROOTDIR)/modules/.config
DEVICE_CONFIG        = $(ROOTDIR)/.config
COMMON_PATH          = $(ROOTDIR)/products/common
DOC_PATH             = $(ROOTDIR)/doc
LEGAL_PATH           = $(ROOTDIR)/legal
LOGDIR		     = $(ROOTDIR)/log
PKG_CONFIG_LIBDIR    = $(STAGING)/lib/
PKG_CONFIG_PATH      = $(STAGING)/lib/pkgconfig
ifeq ($(LICENCE_CHECK), 1)
INOTIFY_SCRIPT       = $(ROOTDIR)/tools/check-oss/inotify.sh
else
INOTIFY_SCRIPT = true
endif

ifeq ($(CONFIG_OEM_VENDOR), y)
ifdef PROD_TUPLE
PROD_PATH            = $(VENDORDIR)/products/$(PROD_TUPLE)
endif

ifneq ($(wildcard $(VENDORDIR)/doc/Makefile),)
DOC_PATH             = $(VENDORDIR)/doc
endif

ifneq ($(wildcard $(VENDORDIR)/legal/Makefile),)
LEGAL_PATH           = $(VENDORDIR)/legal
endif

else
ifdef PROD_TUPLE
PROD_PATH            = $(ROOTDIR)/products/family/$(PROD_TUPLE)
endif
endif

-include $(ROOTDIR)/config.arch

ARCH_PATH            = $(ROOTDIR)/products/arch/$(SYSTEM_ARCH)
MACH_PATH            = $(ARCH_PATH)/mach-$(SYSTEM_MACH)
BOOTLOADER_PATH      = $(REL_REPO)/templates/$(PROD_TUPLE)
BARE_CFG             = config.barebox
LNX_CFG              = config.linux-$(LINUX_MAJOR_VERSION)
MOD_CFG              = config.modules-$(LINUX_MAJOR_VERSION)

CONFIG_BAREBOX	     = $(shell if [ -f $(PROD_PATH)/$(BARE_CFG) ]; then		\
					echo "$(PROD_PATH)/$(BARE_CFG)";	\
		      	      elif [ -f $(MACH_PATH)/$(BARE_CFG) ]; then	\
					echo "$(MACH_PATH)/$(BARE_CFG)";	\
		      	      else						\
					echo "$(ARCH_PATH)/$(BARE_CFG)";	\
			      fi)
CONFIG_LINUX	     = $(shell if [ -f $(PROD_PATH)/$(LNX_CFG) ]; then		\
					echo "$(PROD_PATH)/$(LNX_CFG)";		\
		      	      elif [ -f $(MACH_PATH)/$(LNX_CFG) ]; then		\
					echo "$(MACH_PATH)/$(LNX_CFG)";		\
		      	      else						\
					echo "$(ARCH_PATH)/$(LNX_CFG)";		\
			      fi)


