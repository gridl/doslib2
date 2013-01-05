#if defined(TARGET_WINDOWS)
# include <windows.h>
# include <windows/apihelp.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <hw/cpu/cpu.h>

#include <misc/useful.h>

struct cpu_info_t cpu_info = {
	-1,		/* cpu basic level */
	-1,		/* cpu basic FPU level */
	0,		/* flags */
	0,		/* cpu_type_and_mask */
	NULL		/* cpu_cpuid_info */
};

/* Possible values of cpu_type_and_mask (if it worked):
 *
 *     0x03xx   386DX
 *     0x04xx   486
 *     0x05xx   Pentium
 *     0x23xx   386SX
 *     0x33xx   Intel i376
 *     0x43xx   386SL
 *     0xA3xx   IBM 386SLC
 *     0xA4xx   IBM 486SLC
 *
 *     0x0303   386 B1 to B10, Am386DX/DXL step A
 *     0x0305   Intel D0
 *     0x0308   Intel D1/D2/E1, Am386DX/DXL step B
 *
 *     0x2304   Intel A0
 *     0x2305   Intel B
 *     0x2308   Intel C/D1, Am386SX/SXL step A1
 *     0x2309   Intel 386CX/386EX/386SXstatic step A
 *
 *     For complete list see [http://www.youtube.com/watch?NR=1&feature=endscreen&v=DUiTZVinZEM]
 */

/* CPUID function. To avoid redundant asm blocks */
#if defined(__GNUC__)
# if defined(__i386__) /* Linux GCC + i386 */
/* defined in cpu.h */
# elif defined(__amd64__) /* Linux GCC + x86_64 */
/* TODO */
# endif
#elif TARGET_BITS == 32
/* defined in cpu.c */
void do_cpuid(const uint32_t select,struct cpu_cpuid_generic_block *b) {
	__asm {
		.586p
		pushad
		mov	eax,select
		mov	esi,dword ptr [b]
		mov	ebx,[esi+4]
		mov	ecx,[esi+8]
		mov	edx,[esi+12]
		cpuid
		mov	[esi],eax
		mov	[esi+4],ebx
		mov	[esi+8],ecx
		mov	[esi+12],edx
		popad
	}
}
#elif TARGET_BITS == 16
/* defined in cpu.c */
void do_cpuid(const uint32_t select,struct cpu_cpuid_generic_block FAR *b) {
	__asm {
		.586p
# if defined(__LARGE__) || defined(__COMPACT__) || defined(__HUGE__)
		push	ds
# endif
		pusha
		mov	eax,select
# if defined(__LARGE__) || defined(__COMPACT__) || defined(__HUGE__)
		lds	si,word ptr [b]
# else
		mov	si,word ptr [b]
# endif
		mov	ebx,[si+4]
		mov	ecx,[si+8]
		mov	edx,[si+12]
		cpuid
		mov	[si],eax
		mov	[si+4],ebx
		mov	[si+8],ecx
		mov	[si+12],edx
		popa
# if defined(__LARGE__) || defined(__COMPACT__) || defined(__HUGE__)
		pop	ds
# endif
	}
}
#endif

void cpu_copy_id_string(char *tmp/*must be CPU_ID_STRING_LENGTH or more*/,struct cpu_cpuid_00000000_id_info *i) {
	*((uint32_t*)(tmp+0)) = i->id_1;
	*((uint32_t*)(tmp+4)) = i->id_2;
	*((uint32_t*)(tmp+8)) = i->id_3;
	tmp[12] = 0;
}

char *cpu_copy_ext_id_string(char *tmp/*CPU_EXT_ID_STRING_LENGTH*/,struct cpu_cpuid_info *i) {
	unsigned int ii=0;

	*((uint32_t*)(tmp+0)) = i->e80000002.a;
	*((uint32_t*)(tmp+4)) = i->e80000002.b;
	*((uint32_t*)(tmp+8)) = i->e80000002.c;
	*((uint32_t*)(tmp+12)) = i->e80000002.d;

	*((uint32_t*)(tmp+16)) = i->e80000003.a;
	*((uint32_t*)(tmp+20)) = i->e80000003.b;
	*((uint32_t*)(tmp+24)) = i->e80000003.c;
	*((uint32_t*)(tmp+28)) = i->e80000003.d;

	*((uint32_t*)(tmp+32)) = i->e80000004.a;
	*((uint32_t*)(tmp+36)) = i->e80000004.b;
	*((uint32_t*)(tmp+40)) = i->e80000004.c;
	*((uint32_t*)(tmp+44)) = i->e80000004.d;

	tmp[48] = 0;
	while (ii < 48 && tmp[ii] == ' ') ii++;
	return tmp+ii;
}

static void probe_cpuid() {
	if (cpu_info.cpuid_info != NULL) return;

	cpu_info.cpuid_info = malloc(sizeof(*cpu_info.cpuid_info));
	if (cpu_info.cpuid_info == NULL) return;
	memset(cpu_info.cpuid_info,0,sizeof(*cpu_info.cpuid_info));

	/* alright then, let's ask the CPU it's identification */
	do_cpuid(0x00000000,&(cpu_info.cpuid_info->e0.g)); /* NTS: e0.g aliases over e0.i and fills in info */

	/* if we didn't get anything, then CPUID is not functional */
	if (cpu_info.cpuid_info->e0.i.cpuid_max == 0) {
		cpu_info.cpu_flags &= ~CPU_FLAG_CPUID;
		free(cpu_info.cpuid_info);
		return;
	}

	if (cpu_info.cpuid_info->e0.i.cpuid_max >= 1) {
		do_cpuid(0x00000001,&(cpu_info.cpuid_info->e1.g));
		if (cpu_info.cpuid_info->e1.f.family >= 4 && cpu_info.cpuid_info->e1.f.family <= 6)
			cpu_info.cpu_basic_level = cpu_info.cpuid_info->e1.f.family;

		/* now check for the extended CPUID. it is said that in the original
		 * Pentium ranges 0x80000000-0x8000001F respond exactly the same as
		 * 0x00000000-0x0000001F */
		do_cpuid(0x80000000,&(cpu_info.cpuid_info->e80000000.g));
		if (cpu_info.cpuid_info->e80000000.i.cpuid_max < 0x80000000)
			cpu_info.cpuid_info->e80000000.i.cpuid_max = 0;
		else if (cpu_info.cpuid_info->e80000000.i.cpuid_max >= 0x80000001)
			do_cpuid(0x80000001,&(cpu_info.cpuid_info->e80000001.g));
	}

	/* extended CPU id string */
	if (cpu_info.cpuid_info->e80000000.i.cpuid_max >= 0x80000004) {
		do_cpuid(0x80000002,&(cpu_info.cpuid_info->e80000002));
		do_cpuid(0x80000003,&(cpu_info.cpuid_info->e80000003));
		do_cpuid(0x80000004,&(cpu_info.cpuid_info->e80000004));
	}

	/* modern CPUs report the largest physical address, virtual address, etc. */
	if (cpu_info.cpuid_info->e80000000.i.cpuid_max >= 0x80000008) {
		struct cpu_cpuid_generic_block tmp={0};

		do_cpuid(0x80000008,&tmp);
		cpu_info.cpuid_info->phys_addr_bits = tmp.a & 0xFF;
		cpu_info.cpuid_info->virt_addr_bits = (tmp.a >> 8) & 0xFF;
		cpu_info.cpuid_info->guest_phys_addr_bits = (tmp.a >> 16) & 0xFF;
		if (cpu_info.cpuid_info->guest_phys_addr_bits == 0)
			cpu_info.cpuid_info->guest_phys_addr_bits = cpu_info.cpuid_info->phys_addr_bits;

		cpu_info.cpuid_info->cpu_cores = (tmp.c & 0xFF) + 1;
		if (tmp.c & 0xF000) cpu_info.cpuid_info->cpu_acpi_id_bits = 1 << (((tmp.c >> 12) & 0xF) - 1);
	}
	/* we have to guess for older CPUs */
	else {
		/* If CPU supports long mode, then assume 40 bits */
		if (cpu_info.cpuid_info->e80000001.f.d_lm) {
			cpu_info.cpuid_info->phys_addr_bits = 40;
			cpu_info.cpuid_info->virt_addr_bits = 40;
		}
		/* If CPU supports PSE36, then assume 36 bits */
		else if (cpu_info.cpuid_info->e1.f.d_pse36) {
			cpu_info.cpuid_info->phys_addr_bits = 36;
			cpu_info.cpuid_info->virt_addr_bits = 36;
		}
		else {
			cpu_info.cpuid_info->phys_addr_bits = 32;
			cpu_info.cpuid_info->virt_addr_bits = 32;
		}

		cpu_info.cpuid_info->guest_phys_addr_bits = cpu_info.cpuid_info->phys_addr_bits;
	}
}

static void probe_fpu() {
	unsigned char level=0,flags=0;
	unsigned short tmp=0;

	/* If CPUID is available and reports an FPU, then assume
	 * FPU is present (integrated into the CPU) and do not test.
	 * Carry out the test if CPUID does not report one. */
	if ((cpu_info.cpu_flags & CPU_FLAG_CPUID) && cpu_info.cpuid_info != NULL) {
		if (cpu_info.cpuid_info->e1.f.d_fpu) {
			cpu_info.cpu_basic_fpu_level = cpu_info.cpu_basic_level;
			cpu_info.cpu_flags |= CPU_FLAG_FPU;
			return;
		}
	}

#if defined(TARGET_WINDOWS) && defined(TARGET_WINDOWS_WIN16)
	if (GetWinFlags() & WF_80x87)
		flags |= CPU_FLAG_FPU;
#endif

	if ((flags&CPU_FLAG_FPU) == 0) {
#if defined(__GNUC__)
	/* Linux host: TODO */
#else
		__asm {
			push		ax

			fninit
			mov		word ptr [tmp],0x5A5A
			fnstsw		word ptr [tmp]
			cmp		word ptr [tmp],0
			jnz		no_fpu

			fnstcw		word ptr [tmp]
			mov		ax,word ptr [tmp]
			and		ax,0x103F
			cmp		ax,0x003F
			jnz		no_fpu

/* NTS: Goddamn it Watcom when will your stupid inline assembler support the TWO features
 *      that would make this code so much easier to write:
 *
 *      a) the ability to refer to a member of a structure
 *      b) the ability to use a #define macro as an offset from a symbol
 *
 *      You realize how error-prone it is to maintain this code without those features? */
			or		flags,0x01	; CPU_FLAG_FPU

no_fpu:			pop		ax
		}
#endif
	}

	if (flags & CPU_FLAG_FPU) {
		/* what we assume initially is based on the CPU basic level we detected earlier.
		 * these assumptions are based on typical CPU+FPU hookups and the compatibility
		 * of the chips:
		 *
		 *    8086/8088      can be paired with 8087
		 *    286            can be paired with 80287 (or 80387?)
		 *    386            can be paired with 80287 or 80387
		 *    486            can be paired with 80487 or integrated into the CPU
		 *    Pentium        normally integrated into the CPU
		 *  (anything newer) integrated into the CPU */
		if (cpu_info.cpu_basic_level == 2/*286*/ || cpu_info.cpu_basic_level == 3/*386*/) {
#if defined(__GNUC__)
			level = 3;
			/* Linux host: TODO */
#else
			level = 2;

			__asm {
				.386
				.387

				push		ax
/* 287 vs 387 detection code borrowed from:
 * http://qlibdos32.sourceforge.net/tutor/fpu-detect.asm.txt
 *
 * I have this code execute ONLY on 286 and 386 systems because I'm
 * concerned the test's specific checks might fail on newer processors
 * that do not exactly emulate these small details --J.C.
 *
 * This test can only distinguish a 8087 from 387, it can't exactly detect
 * if it is a 287. So we assume that if it fails the test for a 387, then
 * it's a 287. If it's an 8087, then this code path wouldn't run at all
 * (see "if" statement above)
 *
 * TODO: The test code mentions that it enables FPU interrupts. It also
 *       deliberately divides by zero as part of the test. So far it doesn't
 *       seem to trigger any kind of FPU exception handler, but how do we
 *       make sure that interrupt won't trigger? */
				fstcw		word ptr [tmp]
				and		word ptr [tmp],0xFF7F
				fldcw		word ptr [tmp]
				fdisi
				fstcw		word ptr [tmp]
				fwait
				mov		ax,word ptr [tmp]
				and		ax,0x80
				jz		test_ok2
				jmp		test_done

test_ok2:			finit
				fld1
				fldz
				fdiv
				fld		st(0)
				fchs
				fcompp
				fstsw		ax
				fwait
				and		ah,0x40
				jnz		test_done		; if its zero, then a 387

				/* FIXME: In certain cases, this instruction writes some random memory address?
				          Its not doing it anymore, but it did when this was once coded to jmp
				          to test_done if not, test_ok3 if so. WTF? (also noteworthy: when this
					  code did malfunction the program somehow ended up reporting a 487 FPU
					  when it detected a 386?!?) */
				mov		byte ptr [level],3

test_done:			pop		ax
			}
#endif
		}
		else {
			level = cpu_info.cpu_basic_level;
		}
	}

	cpu_info.cpu_flags |= flags;
	cpu_info.cpu_basic_fpu_level = level;
}

#if defined(__GNUC__)
unsigned int probe_basic_cpu_345_86();
#else
# if TARGET_BITS == 16
unsigned int _cdecl probe_basic_cpu_0123_86();
# endif
unsigned int _cdecl probe_basic_cpu_345_86();
#endif

static void probe_basic_cpu_level() {
	unsigned char level,flags;
	unsigned int dtmp;

#if TARGET_BITS == 32 /* 32-bit DOS, Linux i386, Win32, etc... */
	dtmp = probe_basic_cpu_345_86();
	level = dtmp & 0xFF;
	flags = (dtmp >> 8U) | CPU_FLAG_PROTMODE;
#elif TARGET_BITS == 16
/* ======================= 16-bit realmode DOS / real/protected mode Windows =================== */
	dtmp = probe_basic_cpu_0123_86();
	level = dtmp & 0xFF;
	flags = dtmp >> 8U;

	if (level == 3) {
		dtmp = probe_basic_cpu_345_86();
		level = dtmp & 0xFF;
		flags |= dtmp >> 8U;
	}
#endif /* TARGET_BITS */

	cpu_info.cpu_flags = flags;
	cpu_info.cpu_basic_level = level;
	if (cpu_info.cpu_flags & CPU_FLAG_CPUID)
		probe_cpuid();
}

void probe_cpu() {
	if (cpu_info.cpu_basic_level < 0) {
		probe_basic_cpu_level();
		probe_fpu();
	}
}

