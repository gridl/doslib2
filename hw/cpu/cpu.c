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
		pushad
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
		popad
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

static void probe_basic_cpu_level() {
	unsigned char level,flags;
#if defined(TARGET_WINDOWS) && defined(TARGET_WINDOWS_WIN16)
	DWORD winflags;
#endif

/* 32-bit builds: The fact that we're executing is proof enough the CPU is a 386 or higher, skip the 8086/286 tests.
 * 16-bit builds: Use the Intel Standard test to determine 8086, 286, or up to 386. Also detect "virtual 8086 mode". */
#if TARGET_BITS == 32 /* 32-bit DOS, Linux i386, Win32, etc... */
	level = 3; /* if we're 32-bit code and running then the CPU is definitely a 386 or higher running in protected mode */
	flags = CPU_FLAG_PROTMODE;
#elif TARGET_BITS == 16
/* ======================= 16-bit realmode DOS / real/protected mode Windows =================== */
	level = 0;
	flags = 0;

# if defined(TARGET_WINDOWS) && defined(TARGET_WINDOWS_WIN16)
	winflags = GetWinFlags();
	if (winflags & (WF_PMODE|WF_STANDARD|WF_ENHANCED))
		flags |= CPU_FLAG_PROTMODE;
	if (winflags & 0x2000/*WF_PENTIUM*/)
		level = 5;
	else if (winflags & WF_CPU486)
		level = 4;
	else if (winflags & (WF_ENHANCED|WF_CPU386|WF_PAGING))
		level = 3;
	else if (winflags & WF_CPU286)
		level = 2;
# endif

	if (level == 0) {
		/* an 8086 will always set bits 12-15 */
		__asm {
			pushf
			push	ax

			pushf
			pop	ax

			and	ax,0x0FFF

			push	ax
			popf

			pushf
			pop	ax

			and	ax,0xF000
			cmp	ax,0xF000
			jz	is_8086

			mov	level,2			/* bits 12-15 are not all one, so it must be a 286 */

is_8086:		pop	ax
			popf
		}
	}

	if (level == 2 && (flags&CPU_FLAG_PROTMODE) == 0) {
		/* a 286 will always clear bits 12-15 in real mode.
		 * I don't know how it will react in protected mode, so we skip the test if
		 * targeting Win16 and GetWinFlags() says we're in protected mode */
		__asm {
			pushf
			push	ax

			pushf
			pop	ax

			or	ax,0xF000
				
			push	ax
			popf

			pushf
			pop	ax
				
			and	ax,0xF000
			jz	is_286

			mov	level,3		/* bits 12-15 aren't necessarily zero, so it must be a 386 */

is_286:			pop	ax
			popf
		}
	}

	if (level >= 2) {
		/* if we're on a 286 or higher, use "smsw" to read whether or not the CPU
		 * is in protected mode. on a 386 this is the best way to know whether we're
		 * in real mode vs virtual 8086 mode. unlike "mov eax,cr0" this is legal
		 * to execute in virtual 8086 mode without causing an exception. */
		__asm {
			.286

			push	ax
			smsw	ax
			and	al,1
			shl	al,2			; (1 << 2) == 0x04 == CPU_FLAG_PROTMODE
			or	flags,al
			pop	ax
		}
	}

	if ((flags & CPU_FLAG_PROTMODE) && level >= 3) { /* if protected mode and 386 or higher */
# if defined(TARGET_MSDOS)
		/* 16-bit DOS is supposed to run in real mode. So if CPU_FLAG_PROTMODE is set,
		 * we're in virtual 8086 mode */
		flags |= CPU_FLAG_V86;
# elif defined(TARGET_WINDOWS) && defined(TARGET_WINDOWS_WIN16)
		/* if Windows says we're running in real mode, and yet we see the CPU in
		 * protected mode, then we're (somehow) running under Windows real-mode which
		 * is somehow running under virtual 8086 mode (FIXME: Is that even possible??
		 * TEST: Try Windows 3.0 real mode with EMM386.EXE active---does that trigger this code path?) */
		if ((winflags & (WF_PMODE|WF_STANDARD|WF_ENHANCED)) == 0) flags |= CPU_FLAG_V86;
# endif
	}
#else
# error Unknown TARGET_BITS
#endif /* TARGET_BITS */

	if (level == 3) {
#if defined(__GNUC__)
# if defined(__i386__) /* Linux i386 + GCC */
		unsigned int a;

		/* a 386 will not allow setting the AC bit (bit 18) */
		__asm__(	"pushfl\n"

				"pushfl\n"
				"popl	%%eax\n"
				"or	$0x40000,%%eax\n"
				"pushl	%%eax\n"
				"popfl\n"
				"pushfl\n"
				"popl	%%eax\n"

				"popfl\n"
				: "=a" (a) /* output */ : /* input */ : /* clobbered */);
		if (a&0x40000) level = 4;
# elif defined(__amd64__) /* Linux x86_64 + GCC */
/* TODO */
# endif
#else
		/* a 386 will not allow setting the AC bit (bit 18) */
		/* NTS: Inline assembly: #if TARGET_BITS == 16 doesn't work right, use only #ifdef */
		/* NTS: Do NOT use cli/sti under 32-bit Windows targets. The NT kernel will fault us for it.
		 *      Everyone expects 16-bit code to do it though */
		__asm {
			.386

			pushfd
			push	eax
			push	ebx

# ifdef TARGET_CLI_STI_IS_SAFE
			cli
# endif
			pushfd
			pop	eax

			or	eax,0x40000

			push	eax
			popfd

			pushfd
			pop	eax

			mov	ebx,eax
			and	ebx,0xFFFBFFFF
			push	ebx
			popfd

			test	eax,0x40000
			jz	is_386

			mov	level,4

is_386:			pop	ebx
			pop	eax
			popfd
		}
#endif
	}

	if (level >= 4) {
#if defined(__GNUC__)
# if defined(__i386__) /* Linux i386 + GCC */
		/* if a 486 or higher, check for CPUID */
		unsigned int a,b;

		__asm__(	"pushfl\n"

				"pushfl\n"
				"popl	%%eax\n"
				"and	$0xFFDFFFFF,%%eax\n"
				"pushl	%%eax\n"
				"popfl\n"
				"pushfl\n"
				"popl	%%eax\n"

				"mov	%%eax,%%ebx\n"

				"or	$0x00200000,%%ebx\n"
				"pushl	%%ebx\n"
				"popfl\n"
				"pushfl\n"
				"pop	%%ebx\n"

				"popfl\n"
				: "=a" (a), "=b" (b) /* output */ : /* input */ : /* clobbered */);

		/* a=when we cleared ID  b=when we set ID */
		if ((a&0x00200000) == 0 && (b&0x00200000)) cpu_info.cpu_flags |= CPU_FLAG_CPUID;
# elif defined(__amd64__) /* Linux x86_64 + GCC */
	/* TODO */
# endif
#else
		/* if a 486 or higher, check for CPUID */
		__asm {
			.386

			pushfd
			push	eax

			pushfd
			pop	eax
			and	eax,0xFFDFFFFF
			push	eax
			popfd
			pushfd
			pop	eax
			test	eax,0x200000
			jnz	no_cpuid		; if we failed to clear CPUID then no CPUID

			or	eax,0x200000
			push	eax
			popfd
			pushfd
			pop	eax
			test	eax,0x200000
			jz	no_cpuid		; if we failed to set CPUID then no CPUID

			or	flags,0x08 ; CPU_FLAG_CPUID

no_cpuid:		pop	eax
			popfd
		}
#endif
	}

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

