BUILD_TYPE ?= standalone

ifeq ($(BUILD_TYPE), standalone)
	# export arch, CROSS_COMPILE and kernel source path
endif

# KERNEL_SRC and O are supplied by Yocto module.bbclass:
# KERNEL_SRC=${STAGING_KERNEL_DIR}, O=${STAGING_KERNEL_BUILDDIR}
# M=$(CURDIR) points Kbuild to this external module tree.
modules clean modules_install:
	$(MAKE) -C "$(KERNEL_SOURCE)" O="$(O)" M="$(CURDIR)" CF="-D__CHECK_ENDIAN__" $(@)

.PHONY: modules clean modules_install