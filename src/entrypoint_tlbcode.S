#define N64_TLBCODE_BOOT 1
#define _start _start_tlbcode
#include "entrypoint.S"

    .section .intvectors.tlbcode, "ax"
    .set noreorder
    .macro mapped_vector
    lui k0, %hi(inthandler)
    addiu k0, k0, %lo(inthandler)
    jr k0
    nop
    .endm
    .org 0
    mapped_vector
    .org 0x80
    mapped_vector
    .org 0x100
    mapped_vector
    .org 0x180
    mapped_vector
