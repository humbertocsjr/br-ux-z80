
DISKA_ID: equ 0xc0ea
DISKB_ID: equ 0xc0eb
DISKC_ID: equ 0xc0ec
DISKD_ID: equ 0xc0ed
DISKA_MEDIA: equ 0xc0fa
DISKB_MEDIA: equ 0xc0fb
DISKC_MEDIA: equ 0xc0fc
DISKD_MEDIA: equ 0xc0fd

    org 0xc000
    db 0xeb, 0xfe, 0x90, 'B'
    db 0
    db 0xf9
    times 0xc01e-$ db 0
    ld hl, 0xe000
    ld sp, hl
    call print
    db 27,69,"Booting .",0
    ld b, 5
    ld a, 0xf9
    ld (DISKA_MEDIA), a
    ld c, a
    ld hl, 0xc100
    ld de, 1
    ld a, 255
    ld (DISKB_ID), a
    ld (DISKC_ID), a
    ld (DISKD_ID), a
    xor a
    ld (DISKA_ID), a
    call 0x144
    jp nc, 0xc100
    call print
    db " [FAIL]",13,10,0
    jp $
print:
    pop hl
    .loop:
        ld a, (hl)
        inc hl
        cp 0
        jr z, .end
        call 0xa2
        jr .loop
    .end:
    push hl
    ret
