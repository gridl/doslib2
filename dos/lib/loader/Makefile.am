
BUILD_dos16r=1

-include ../../../config.mak
-include Makefile.wcc

debugme:
	@echo Enabled CPU: $(BUILD_enabled_cpus)
	@echo Enabled MM: $(BUILD_enabled_mm)
	@echo Targets: $(BUILD_targets)
	@echo suffixes "$(suffixes)"

all:
	+@for i in "" $(BUILD_targets); do\
		if [ x"$$i" != x ]; then \
			$(MAKE) --no-print-directory -$(MAKEFLAGS) all-targets target_subdir=$$i || break;\
		fi;\
	done

clean:
	@rm -f *~
	+@for i in "" $(BUILD_targets); do\
		if [ x"$$i" != x ]; then \
			$(MAKE) --no-print-directory -$(MAKEFLAGS) clean-targets target_subdir=$$i || break;\
		fi;\
	done

disk:
	dd if=/dev/zero of=test.dsk bs=512 count=2880
	mkfs.vfat test.dsk
	mcopy -i test.dsk ../../build/hw/cpu/win16b/20_386l/exe/test1.exe ::b20_386l.exe || true
	mcopy -i test.dsk ../../build/hw/cpu/win16b/20_386c/exe/test1.exe ::b20_386c.exe || true
	mcopy -i test.dsk ../../build/hw/cpu/win16b/30_386l/exe/test1.exe ::b30_386l.exe
	mcopy -i test.dsk ../../build/hw/cpu/win16b/30_386c/exe/test1.exe ::b30_386c.exe
	mcopy -i test.dsk ../../build/hw/cpu/win16b/31_386l/exe/test1.exe ::b31_386l.exe
	mcopy -i test.dsk ../../build/hw/cpu/win16b/31_386c/exe/test1.exe ::b31_386c.exe
	mcopy -i test.dsk ../../build/hw/cpu/dos16r/086l/exe/test1.exe ::d16_086l.exe
	mcopy -i test.dsk ../../build/hw/cpu/dos16r/086c/exe/test1.exe ::d16_086c.exe
	mcopy -i test.dsk ../../build/hw/cpu/dos32p/386f/exe/test1.exe ::d32_386f.exe
	mcopy -i test.dsk ../../build/hw/cpu/win32s/31_386f/exe/test1.exe ::win32s.exe
	mcopy -i test.dsk ../../build/hw/cpu/win32/95_386f/exe/test1.exe ::win95.exe
	mcopy -i test.dsk ../../build/hw/cpu/win32/nt_386f/exe/test1.exe ::winnt.exe
	mcopy -i test.dsk ../../build/hw/cpu/winwa386/31_386f/exe/test1.exe ::win386.exe
	mcopy -i test.dsk ../../dos4gw.dat ::dos4gw.exe

