; MSX
CHPUT: equ 0xa2
; BR-UX
KERNEL_DATA_SLOT: equ 0xc0ff
MEGARAM_SLOT: equ 0xc0fe
KERNEL_DATA: equ 0xa000
DISKA_ID: equ 0xc0ea
DISKB_ID: equ 0xc0eb
DISKC_ID: equ 0xc0ec
DISKD_ID: equ 0xc0ed
DISKA_MEDIA: equ 0xc0fa
DISKB_MEDIA: equ 0xc0fb
DISKC_MEDIA: equ 0xc0fc
DISKD_MEDIA: equ 0xc0fd
KERNEL_PROCESS_ID: equ 0xfe
    org 0xc100
pre_init:
    ; Replace bootloader
    ld hl, 0x4000
    ld de, 0xc100
    ld bc, end - pre_init
    ldir
    jp init
init:
    call kprint
    db 27,69
    db "BR-UX ", VERSION + '0', EDITION, " v", VERSION + '0', ".", SUB_VERSION + '0', " Operating System",13,10
    db "Copyright (c) 2024, Humberto Costa",13,10,0
    
    call kprint_horiz
    
    ; Initialize multitask
    call proc_init

    ; Initialize Memory Manager
    call memmgr_init

    ; Reserve Memory page for Kernel Data
    call kprint
    db "Allocating Kernel Data.", 0
    ld a, KERNEL_PROCESS_ID
    call memmgr_alloc_page
    jp nc, .fail
    ld (KERNEL_DATA_SLOT), a
    ld hl, KERNEL_DATA_SLOT
    call memmgr_set_page_a000
    call kprint
    db " [ OK ]", 0





    jp $
    .fail:
        call kprint
        db " [ FAIL ]",0
        jp $


include "modules/proc.s"
include "modules/fs.s"
include "modules/pty.s"
include "modules/memmgr.s"
include "modules/fd.s"
include "../include/osver.s"

end: