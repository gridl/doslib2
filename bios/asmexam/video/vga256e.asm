;--------------------------------------------------------------------------------------
; VGA256E.COM
; 
; Setup VGA 320x200x256 and display a bitmap. This time it uses "Mode X" to write to VRAM.
; Then uses write mode 2 to vertically stretch and scale.
;--------------------------------------------------------------------------------------
		bits 16			; 16-bit real mode

		segment text class=code

..start:	push	cs
		pop	ds

; load bitmap from file into memory
		mov	ax,0x3D00	; AH=0x3D OPEN FILE AL=0 compat sharing
		mov	dx,str_bmp	; DS:DX = path to file
		int	21h
		jnc	open_ok

		mov	ax,0x4C01
		int	21h
		hlt
open_ok:	mov	[cs:handle],ax

; read data into memory
		mov	ah,0x3F
		mov	bx,[cs:handle]
		mov	dx,seg bitmap_hdr
		mov	ds,dx
		mov	dx,bitmap_hdr
		mov	cx,(320*200) + (256*4) + 0x36
		int	21h

; close file
		mov	ah,0x3E
		mov	bx,[cs:handle]
		int	21h

; setup the mode
		mov	ax,19		; mode 0x13 VGA 320x200x256
		int	10h

		push	cs
		pop	ds
		mov	word [crt_port],0x3D4
		mov	word [status_port],0x3DA
		int	11h
		and	al,0x30
		cmp	al,0x30
		jne	not_mono
		mov	word [crt_port],0x3B4
		mov	word [status_port],0x3BA
not_mono:

		mov	dx,0x3C4
		mov	al,0x04
		out	dx,al		; address reg 4 of the sequencer
		inc	dx
		mov	al,0x06		; Chain 4 disable, extended memory enable, disable odd/even
		out	dx,al
		dec	dx

		mov	al,0x02		; now reg 2
		out	dx,al
		inc	dx
		mov	al,0x0F		; enable all planes
		out	dx,al

		mov	dx,0x3D4

		mov	al,0x14
		out	dx,al
		inc	dx
		mov	al,0x00		; write CRTC reg 0x14 turn off long mode
		out	dx,al
		dec	dx

		mov	al,0x17
		out	dx,al
		inc	dx
		mov	al,0xE3		; write CRTC reg 0x17 enable byte mode
		out	dx,al
		dec	dx

; load the color palette
		cld
		mov	si,seg bitmap_palette
		mov	ds,si
		mov	si,bitmap_palette
		mov	cx,256
		mov	dx,0x3C8
		xor	al,al
		out	dx,al
		inc	dx

palette_load:	push	cx
		mov	cl,2

		lodsb
		shr	al,cl
		push	ax

		lodsb
		shr	al,cl
		push	ax

		lodsb
		shr	al,cl
		out	dx,al
		pop	ax
		out	dx,al
		pop	ax
		out	dx,al

		lodsb

		pop	cx
		loop	palette_load

		cld
		mov	ax,0xA000
		mov	es,ax

; draw top part
		mov	byte [cs:bitmask],0x11
		mov	si,bitmap
drawloop:	mov	dx,0x3C4
		mov	al,0x02
		out	dx,al

		inc	dx
		mov	al,[cs:bitmask]
		out	dx,al

		xor	di,di		; start at top
		mov	ax,seg bitmap
		mov	ds,ax
		mov	cx,(320*200)/4	; 80*200 groups
		push	si
.innerloop:	lodsb
		add	si,3
		stosb
		loop	.innerloop
		pop	si

		inc	si
		shl	byte [cs:bitmask],1
		jnc	drawloop

; wait for user
		xor	ax,ax
		int	16h

; flip the screen to another part of VRAM
		mov	dx,[cs:crt_port]

		mov	al,0x0C		; CRT start address (high)
		out	dx,al
		inc	dx
		mov	al,(80*200) >> 8
		out	dx,al
		dec	dx

		mov	al,0x0D		; CRT start address (low)
		out	dx,al
		inc	dx
		mov	al,(80*200) & 0xFF
		out	dx,al
		dec	dx

; setup write mode 1
		mov	ax,0x4905	; write reg 5 = (read mode 1, write mode 1)
		call	gc_write

		mov	dx,0x3C4
		mov	al,0x02
		out	dx,al
		inc	dx
		mov	al,0x0F
		out	dx,al

; now use write mode 1 to bitblt one part of the screen to another
scrollanim:	call	vblank

		mov	cx,200
		xor	si,si
		mov	di,(80*200)
		xor	bx,bx
.scaleloop:	call	copyline
		add	bx,[cs:scale]
.scaleloopadv:	cmp	bx,0x100
		jb	.scalenextline
		sub	bx,0x100
		add	si,80
		jmp	short .scaleloopadv
.scalenextline:	add	di,80
		loop	.scaleloop

		mov	ax,[cs:scale]
		add	ax,[cs:scale_dir]
		cmp	ax,0
		jle	.scale_adj
		cmp	ax,0x0200
		jge	.scale_adj
		mov	[cs:scale],ax
		jmp	short .scale_adj_done
.scale_adj:	neg	word [cs:scale_dir]
.scale_adj_done:

		mov	ah,1		; any key press?
		int	16h
		jz	scrollanim	; loop if not
		xor	ax,ax
		int	16h		; eat the key

; restore text mode
		mov	ax,3
		int	10h

; exit
		mov	ax,0x4C00
		int	21h

; data
str_bmp:	db	'vga256p1.200',0
crt_port:	dw	1
status_port:	dw	1
scale:		dw	0x100
scale_dir:	dw	1
handle:		resw	1
bitmask:	resb	1

; copy scanline in VRAM from SI to DI, 80 bytes
copyline:	push	cx
		push	si
		push	di
		push	ds
		push	es
		pop	ds
		cld

		mov	cx,80
		rep	movsb

		pop	ds
		pop	di
		pop	si
		pop	cx
		ret

vblank:		; first wait for vblank to finish
		mov	dx,[cs:status_port]

.l1:		in	al,dx
		test	al,0x08
		jnz	.l1

		; then wait for vblank
.l2:		in	al,dx
		test	al,0x08
		jz	.l2
		
		ret

; AH=data to write AL=register index
gc_write:	mov	dx,0x3CE
		out	dx,al
		inc	dx
		mov	al,ah
		out	dx,al
		ret

		segment data1

; NTS: The bitmap totals 112KB, which Watcom will not allow to fit in one segment.
;      So we split it up into a top half and bottom half across two segments.
;      The image is 16-color (4-bit) packed. The includes here assume a bitmap
;      saved by the GIMP in Windows BMP format that is 640x350 16-color 4-bit packed.
;      This version assumes the default EGA palette and does not include it from the bitmap.
bitmap_hdr:	resb	0x36
bitmap_palette:	resb	256*4
bitmap:		resb	320*200

		segment stack class=stack

		resw		1024

		segment bss

