_memmgr_table:
    times 256 db 0xff
_memmgr_total_pages:
    db 0
_memmgr_total_kib:
    dw 0
_memmgr_curr_4000:
    db 0
_memmgr_curr_6000:
    db 0
_memmgr_curr_8000:
    db 0
_memmgr_curr_a000:
    db 0

memmgr_init:
    call kprint
    db "Detecting MRAM Size",0

    ld a, 15
    call _memmgr_check
    jp nc, .fail
    ld a, 16
    ld (_memmgr_total_pages), a
    ld hl, 128
    ld (_memmgr_total_kib), hl
    
    ld a, 31
    call _memmgr_check
    jp nc, .continue
    ld a, 32
    ld (_memmgr_total_pages), a
    ld hl, 256
    ld (_memmgr_total_kib), hl
    
    ld a, 63
    call _memmgr_check
    jp nc, .continue
    ld a, 64
    ld (_memmgr_total_pages), a
    ld hl, 512
    ld (_memmgr_total_kib), hl
    
    ld a, 127
    call _memmgr_check
    jp nc, .continue
    ld a, 128
    ld (_memmgr_total_pages), a
    ld hl, 1024
    ld (_memmgr_total_kib), hl
    
    ld a, 255
    call _memmgr_check
    jp nc, .continue
    ld a, 255
    ld (_memmgr_total_pages), a
    ld hl, 2048
    ld (_memmgr_total_kib), hl

    .continue:
    call kprint
    db " [ ",0
    ld hl, (_memmgr_total_kib)
    call kprint_uint
    call kprint
    db " KiB ]",13,10,0
    ld b, (_memmgr_total_pages)
    ld hl, _memmgr_table
    .fill:
        xor a
        ld (hl), a
        inc hl
        djnz .fill
    ret
    .fail:
        call kprint_fail
        jp $

; a = page
_memmgr_check:
    call kprint_dot
    push bc
    push hl
    push af
    ld b, a
    xor a
    ld hl, 0x4000
    call memmgr_set_page_4000
    ld (hl), 0xaa
    pop af
    call memmgr_set_page_4000
    ld (hl), 0xbb
    xor a
    call memmgr_set_page_4000
    ld a, 0xaa
    cp (hl)
    ld a, b
    pop hl
    pop bc
    jr nz, .fail
    scf
    ret
    .fail:
        scf
        ccf
        ret

; a = page
memmgr_set_page_4000:
    push af
    push hl
    out (0x8e), a
    ld (_memmgr_curr_4000), a
    ld hl, 0x4000
    ld (hl), a
    in a,(0x8e)
    pop hl
    pop af
    ret
    
; a = page
memmgr_set_page_6000:
    push af
    push hl
    out (0x8e), a
    ld (_memmgr_curr_6000), a
    ld hl, 0x6000
    ld (hl), a
    in a,(0x8e)
    pop hl
    pop af
    ret

; a = page
memmgr_set_page_8000:
    push af
    push hl
    out (0x8e), a
    ld (_memmgr_curr_8000), a
    ld hl, 0x8000
    ld (hl), a
    in a,(0x8e)
    pop hl
    pop af
    ret
; a = page
memmgr_set_page_a000:
    push af
    push hl
    out (0x8e), a
    ld (_memmgr_curr_a000), a
    ld hl, 0xa000
    ld (hl), a
    in a,(0x8e)
    pop hl
    pop af
    ret

; a = process id
; ret: a = page
memmgr_alloc_page:
    push hl
    push bc
    push de
    ld e, a
    ld b, (_memmgr_total_pages)
    ld c, 0
    ld hl, _memmgr_table
    .find:
        ld a, (hl)
        cp 0
        jr z, .found
        inc hl
        inc c
        djnz .find
    pop de
    pop bc
    pop hl
    xor a
    scf
    ccf
    ret
    .found:
    ld (hl), e
    ld a, c
    pop de
    pop bc
    pop hl
    scf
    ret