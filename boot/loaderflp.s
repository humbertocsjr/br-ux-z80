; MegaRAM Map
; 0 = Kernel Data at 0xa000
; 1 = Kernel at 0x4000
LINL40: equ 0xf3ae
INITXT: equ 0x6c
PHYDIO: equ 0x144
ENASLT: equ 0x24
CHPUT: equ 0xa2
MEGARAM: equ 0xc0fe
KERNEL_DATA: equ 0xa000
DISKA_ID: equ 0xc0ea
DISKB_ID: equ 0xc0eb
DISKC_ID: equ 0xc0ec
DISKD_ID: equ 0xc0ed
DISKA_MEDIA: equ 0xc0fa
DISKB_MEDIA: equ 0xc0fb
DISKC_MEDIA: equ 0xc0fc
DISKD_MEDIA: equ 0xc0fd
BRUXFS_INDEX: equ 6
BLOCK_ID: equ 500
BLOCK_MODE: equ 502
BLOCK_NEXT: equ 504
BLOCK_CHILD: equ 506
BLOCK_SIZE: equ 510
    org 0xc100
    ld a, 40
    ld (LINL40), a
    call INITXT
    call print
    db 27,69
    db "BR-UX ",VERSION+'0', EDITION, " System Loader",13,10
    db 1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57
    db 1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57,1,0x57
    db 1,0x57,1,0x57
    db 13,10,10,0
    ; Detect MegaRAM Slot
    call print
    db "Detecting MegaRAM",0
    ; Try slot id 1
    ld a, 1
    call megaram_detect
    jp c, .continue
    ; Try slot id 2
    ld a, 2
    call megaram_detect
    jp nc, .fail
    .continue:
    ; Reset MegaRAM page
    ld a, 0
    ld h, 0xa0
    call megaram_set_page
    ld a, 1
    ld h, 0x40
    call megaram_set_page
    ld a, 2
    ld h, 0x60
    call megaram_set_page
    ld a, 3
    ld h, 0x80
    call megaram_set_page
    ; Mount BRUXFS
    call print
    db "Loading BR-UX Kernel",0
    ; Read Index/Root Block
    ld de, BRUXFS_INDEX
    call diska_read
    jp nc, .fail
    ; Check if Root has chilren
    ld de, (KERNEL_DATA+BLOCK_CHILD)
    ld a, e
    or d
    jp z, .fail
    ; Load first child
    call diska_read
    jp nc, .fail
    ; Find kernel
    .find:
        ld hl, KERNEL_DATA
        ld de, kernel_name
        call streq
        jp z, .found
        ld de, (KERNEL_DATA+BLOCK_NEXT)
        ld a, e
        or d
        jp z, .fail
        call diska_read
        jp nc, .fail
        jr .find
    .found:
    ; Check if kernel has chilren
    ld de, (KERNEL_DATA+BLOCK_CHILD)
    ld a, e
    or d
    jp z, .fail
    ; Load first child
    call diska_read
    jp nc, .fail
    ld de, 0x4000
    .loop:
        ld hl, KERNEL_DATA
        ld bc, 500
        ldir
        ld hl, (KERNEL_DATA+BLOCK_NEXT)
        ld a, l
        or h
        jp z, .done
        push de
        ld de, (KERNEL_DATA+BLOCK_NEXT)
        call diska_read
        pop de
        jp nc, .fail
        jp .loop
    .done:
    call print
    db " [DONE]",13,10,0

    jp $
    .fail:
        call print
        db " [FAIL]",0
        jp $

; hl = str1
; de = str2
streq:
    push hl
    push de
    push bc
    ld b, a
    .loop:
        ld a, (de)
        cp (hl)
        jr nz, .end
        cp 0
        jr z, .end
        inc hl
        inc de
        jr .loop
    .end:
    ld a, b
    pop bc
    pop de
    pop hl
    ret


; de = block
; ret a = sectors read
;     cf = 1=ok
diska_read:
    push hl
    push bc
    push de
    ld a, (DISKA_MEDIA)
    ld c, a
    ld b, 1
    ld a, (DISKA_ID)
    ld hl, KERNEL_DATA
    scf
    ccf
    call PHYDIO
    call print
    db " .",0
    ld a, b
    pop de
    pop bc
    pop hl
    ccf
    ret

; a = page
; h = segment (0x40/0x60/0x80/0xa0)
megaram_set_page:
    push af
    out (0x8e), a
    ld (hl), a
    in a,(0x8e)
    pop af
    ret

; a = slot
megaram_detect:
    ld (MEGARAM), a
    ld h, 0x40
    call ENASLT
    ld a, (MEGARAM)
    ld h, 0x80
    call ENASLT
    call print
    db " .",0
    in a, (0x8e)
    ld a, (0x4000)
    ld b, a
    inc a
    ld (0x4000), a
    cp b
    jr z, .rom
    ; Try write 0x94 to page 1
    out (0x8e), a
    ld a, 1
    ld (0x4000), a
    in a, (0x8e)
    ld a, 0x94
    ld (0x4000), a
    ; Try write 0x49 to page 2
    out (0x8e), a
    ld a, 2
    ld (0x4000), a
    in a, (0x8e)
    ld a, 0x49
    ld (0x4000), a
    ; Try read 0x94 from page 1
    out (0x8e), a
    ld a, 1
    ld (0x4000), a
    in a, (0x8e)
    ld a, (0x4000)
    cp 0x94
    jr nz, .rom
    ; Try read 0x49 from page 2
    out (0x8e), a
    ld a, 2
    ld (0x4000), a
    in a, (0x8e)
    ld a, (0x4000)
    cp 0x49
    jr nz, .rom
    call print
    db " (Slot #",0
    ld a, (MEGARAM)
    ld b, '0'
    add a, b
    call CHPUT
    call print
    db ") [OK]",13,10,0
    scf
    ret
    .rom:
        scf
        ccf
        ret

print:
    pop hl
    push af
    .loop:
        ld a, (hl)
        inc hl
        cp 0
        jr z, .end
        call CHPUT
        jr .loop
    .end:
    pop af
    push hl
    ret

include "../include/osver.s"

kernel_name:
    db "brux",0