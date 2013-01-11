%ifdef TARGET_MSDOS
 %if TARGET_BITS == 16

struc _dos_dpmi_state
	.flags			resb	1	; +0 unsigned char	flags;
	.entry_ip		resw	1	; +1 unsigned short	entry_ip;
	.entry_cs		resw	1	; +3 unsigned short	entry_cs;
	.dpmi_private_size	resw	1	; +5 unsigned short	dpmi_private_size;
	.dpmi_version		resw	1	; +7 unsigned short	dpmi_version;
	.dpmi_processor		resb	1	; +9 unsigned char	dpmi_processor;
	.dpmi_private_segment	resw	1	; +10 unsigned short	dpmi_private_segment;
endstruc

extern dos_dpmi_state;

 %define DPMI_SERVER_PROBED		0x01
 %define DPMI_SERVER_PRESENT		0x02
 %define DPMI_SERVER_INIT		0x04
 %define DPMI_SERVER_CAN_DO_32BIT	0x08
 %define DPMI_SERVER_INIT_32BIT		0x10

 %endif
%endif
