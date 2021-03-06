
rel_srcdir=$(subst $(abs_top_srcdir)/,,$(abs_srcdir))
ifneq ($(target_subdir),)
abs_exe_dir=$(abs_top_builddir)/build/$(rel_srcdir)/$(target_subdir)/exe
abs_obj_dir=$(abs_top_builddir)/build/$(rel_srcdir)/$(target_subdir)/obj
abs_lib_dir=$(abs_top_builddir)/build/$(rel_srcdir)/$(target_subdir)/lib

ifeq ($(target_subdir),linux/i686)
target_subdir_is_linux=1
endif
ifeq ($(target_subdir),linux/x86_64)
target_subdir_is_linux=1
endif

ifeq ($(target_subdir),efi/ia32)
target_subdir_is_efi=1
endif
ifeq ($(target_subdir),efi/x64)
target_subdir_is_efi=1
endif

linux_host_mach=$(shell uname -m)

$(abs_exe_dir):
	@mkdir -p $@

$(abs_lib_dir):
	@mkdir -p $@

$(abs_obj_dir):
	@mkdir -p $@
endif

# what CPUs were asked to be built?
BUILD_enabled_cpus=
ifeq ($(ENABLE_cpu_8086),1)
 BUILD_enabled_cpus += 0
endif
ifeq ($(ENABLE_cpu_286),1)
 BUILD_enabled_cpus += 2
endif
ifeq ($(ENABLE_cpu_386),1)
 BUILD_enabled_cpus += 3
endif
ifeq ($(ENABLE_cpu_486),1)
 BUILD_enabled_cpus += 4
endif
ifeq ($(ENABLE_cpu_586),1)
 BUILD_enabled_cpus += 5
endif
ifeq ($(ENABLE_cpu_686),1)
 BUILD_enabled_cpus += 6
endif

# what memory models?
BUILD_enabled_mm=
ifeq ($(ENABLE_mm_compact),1)
 BUILD_enabled_mm += compact
endif
ifeq ($(ENABLE_mm_small),1)
 BUILD_enabled_mm += small
endif
ifeq ($(ENABLE_mm_medium),1)
 BUILD_enabled_mm += medium
endif
ifeq ($(ENABLE_mm_large),1)
 BUILD_enabled_mm += large
endif
ifeq ($(ENABLE_mm_huge),1)
 BUILD_enabled_mm += huge
endif
ifeq ($(ENABLE_mm_flat),1)
 BUILD_enabled_mm += flat
endif

# what Windows versions (TODO: add as configure options too: 10 20 30 31 95 98 me nt)
BUILD_enabled_windows=
ifeq ($(ENABLE_windows_10),1)
 BUILD_enabled_windows += 10
endif
ifeq ($(ENABLE_windows_20),1)
 BUILD_enabled_windows += 20
endif
ifeq ($(ENABLE_windows_30),1)
 BUILD_enabled_windows += 30
endif
ifeq ($(ENABLE_windows_31),1)
 BUILD_enabled_windows += 31
endif
ifeq ($(ENABLE_windows_95),1)
 BUILD_enabled_windows += 95
endif
ifeq ($(ENABLE_windows_98),1)
 BUILD_enabled_windows += 98
endif
ifeq ($(ENABLE_windows_me),1)
 BUILD_enabled_windows += me
endif
ifeq ($(ENABLE_windows_nt),1)
 BUILD_enabled_windows += nt
endif

# EFI targets
BUILD_efi=
ifeq ($(ENABLE_efi_ia32),1)
 BUILD_efi += ia32
endif
ifeq ($(ENABLE_efi_x64),1)
 BUILD_efi += x64
endif

# list target subdirs
BUILD_targets=

# dos16r
include $(abs_top_builddir)/targets/auto.dos16r.mak

# dos32p
include $(abs_top_builddir)/targets/auto.dos32p.mak

# win16r
include $(abs_top_builddir)/targets/auto.win16r.mak

# win16p
include $(abs_top_builddir)/targets/auto.win16p.mak

# win16b
include $(abs_top_builddir)/targets/auto.win16b.mak

# win32s
include $(abs_top_builddir)/targets/auto.win32s.mak

# winwa386
include $(abs_top_builddir)/targets/auto.winwa386.mak

# win32
include $(abs_top_builddir)/targets/auto.win32.mak

# linux-host
include $(abs_top_builddir)/targets/auto.linuxhost.mak

# efi
include $(abs_top_builddir)/targets/auto.efi.ia32.mak
include $(abs_top_builddir)/targets/auto.efi.x64.mak

ifneq ($(target_subdir),)
# Yes, I'm aware $(target_subdir) usually takes the form dos16r/086c
include $(abs_top_builddir)/targets/config.$(patsubst %/,%,$(dir $(target_subdir))).mak
endif

