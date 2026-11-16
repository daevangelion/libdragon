############################################################
# Libdragon entrypoint
#
# This is the entrypoint for all Libdragon applications. It
# is the first code run after the IPL3.
#
# Authors: Jennifer Taylor <dragonminded@dragonminded.com>
#          Giovanni Bajo <giovannibajo@gmail.com>
############################################################

#include "regs.S"
#include "emux.inc"

	.set noreorder

#ifdef N64_TLBCODE_BOOT
	.section .boot, "ax"
#else
	.section .boot
#endif
	.global _start
_start:
	/* initialize small data pointer */
	la gp, _gp

	/* a bit from libgloss so we start at a known state */
	li v0,SR_CU1|SR_PE|SR_FR|SR_KX|SR_SX|SR_UX
	mtc0 v0,C0_SR


#ifdef N64_TLBCODE_BOOT
	/* Invalidate every slot with a distinct, non-overlapping VPN2. */
	li t0, 31
	li t1, 0x4003E000       /* invalid VPN2 = 0x40000000 + index * 8192 */
	mtc0 zero, $5
	mtc0 zero, $2
	mtc0 zero, $3
1:
	mtc0 t0, $0
	mtc0 t1, $10
	nop
	nop
	tlbwi
	nop
	addiu t1, t1, -8192
	bnez t0, 1b
	addiu t0, t0, -1

	/* 1 MiB pages: one wired entry covers a 2 MiB resident range. */
	li t0, 0
	li t1, 0xC0000000
	li t2, 0x1F              /* G, V, D, cacheable noncoherent */
	li t3, 0x001FE000
	mtc0 t3, $5
	la t4, __tlbcode_wired
	mtc0 t4, $6
2:
	mtc0 t0, $0
	mtc0 t1, $10
	mtc0 t2, $2
	li t3, 0x4000           /* next 1 MiB PFN */
	addu t3, t2, t3
	mtc0 t3, $3
	nop
	nop
	tlbwi
	nop
	addiu t0, t0, 1
	li t3, 0x200000
	addu t1, t1, t3
	li t3, 0x8000
	addu t2, t2, t3
	bne t0, t4, 2b
	nop
	mtc0 zero, $5
	mtc0 zero, $10
	nop
	nop
#endif
#if 0
	/* Run a check to verify that BSS is cleared. This is useful while debugging
	   IPL3 changes. */
	jal __bss_check
	nop
#endif

	/* Copy bootcode flags, stored in DMEM */
	lui v0, 0xA400
	lw t0, 0x0000(v0)
	lw t1, 0x0004(v0)
	lhu t2, 0x000C(v0)
	sw t0, %gprel(__boot_memsize)(gp)
	sw t1, %gprel(__entropy_state+0)(gp)
	sw t1, %gprel(__entropy_state+4)(gp)
	sw t2, %gprel(__boot_address_page)(gp)

	/* Copy other bootcode flags */
	lbu t0, 0x0009(v0)
	lbu t1, 0x000A(v0)
	lbu t2, 0x000B(v0)
	sw t0, %gprel(__boot_tvtype)(gp)
	sw t1, %gprel(__boot_resettype)(gp)
	sw t2, %gprel(__boot_consoletype)(gp)

	la t0, debug_assert_func    /* install assert function in system.c */
	la t1, __assert_func_ptr
	sw t0, 0(t1)	

#ifdef N64_TLBCODE_BOOT
	la t9, __do_global_ctors
	jalr t9
	nop
	li a0, 0
	la t9, main
	jalr t9
	li a1, 0
#else
	jal __do_global_ctors		/* call global constructors */
	nop
	li a0, 0
	jal main					/* call main app */
	li a1, 0
#endif

_abort:
	xioctl_exit				/* request the emulator to exit */
1:	j 1b
	nop
