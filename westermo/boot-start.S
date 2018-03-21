.global _start
_start:
	// the stack is setup at the end of SRAM by the bootrom
	push	{lr}
	bl	pbl_main
	pop	{pc}

.global _drg_spi_copy
_drg_spi_copy:
	// arg0 (r0) contains base address of dragonite ITCM
	adr	r1, _drg_spi_start
	adr	r2, _drg_spi_end
1:
	cmp	r1, r2
	bxeq	lr
	ldr	r3, [r1]
	str	r3, [r0]
	add	r1, #4
	add	r0, #4
	b	1b
	
_drg_spi_start:
	.incbin	"westermo/drg-spi.bin"
_drg_spi_end:
