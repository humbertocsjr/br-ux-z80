
; de = inode
; hl = source
; bc = len
pty_write:
    push af
    push hl
    push bc
    push de
    .loop:
        ld a, b
        or c
        jr z, .end
        ld a, (hl)
        inc hl
        dec bc
        call CHPUT
        jr .loop
    .end:
    pop de
    pop bc
    pop hl
    pop af
    ret

kprint_horiz:
    push hl
    push bc
    push af
    ld b, 40
    .loop:
        ld a, 1
        call CHPUT
        ld a, 0x57
        call CHPUT
        djnz .loop
    pop af
    pop bc
    pop hl
    ret

; ip = asciz string
kprint:
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

kprint_uint:
    push hl
    push de
    push af
    push bc
    ld b, 0
    ld de, 10000
    call .digit
    ld de, 1000
    call .digit
    ld de, 100
    call .digit
    ld de, 10
    call .digit
    ld b, 1
    ld de, 1
    call .digit
    pop bc
    pop af
    pop de
    pop hl
    ret
    .digit:
        ld c, '0'
        .loop:
            xor a
            sbc hl, de
            jp c, .done
            inc c
            ld b, 1
            jr .loop
        .done:
        add hl, de
        xor a
        cp b
        ret z
        ld a, c
        call CHPUT
        ret


kprint_ok:
    call kprint
    db " [ OK ]",13,10,0
    ret

kprint_dot:
    call kprint
    db ".",0
    ret

kprint_fail:
    call kprint
    db " [ FAIL ]",13,10,0
    ret