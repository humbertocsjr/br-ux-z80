
obj_proc: equ 16
    .id: equ 0
    .status: equ 1
    .code_seg: equ 2
    .data_seg: equ 4
    .stack_seg: equ 6
    .stack_top: equ 8
    .parent_id: equ 12
    .next_ptr: equ 14
qty_proc: equ 64

PROC_EMPTY: equ 0
PROC_RUNNING: equ 1
PROC_WAITING: equ 2
PROC_SENDING: equ 3

_procs: 
    times obj_proc * qty_proc db 0

_curr_proc: dw _procs

proc_init:
    ld b, qty_proc
    ld a, 1
    ld iy, _procs
    ld de, obj_proc
    .loop:
        ld (iy+obj_proc.status), PROC_EMPTY
        ld (iy+obj_proc.code_seg), 0
        ld (iy+obj_proc.data_seg), 0
        ld (iy+obj_proc.stack_seg), 0
        ld (iy+obj_proc.stack_top), 0xa000
        ld (iy+obj_proc.id), a
        push iy
        pop hl
        ld (iy+obj_proc.next_ptr), l
        ld (iy+obj_proc.next_ptr+1), h
        inc a
        add iy, de
        djnz .loop
    ld iy, _procs
    ld (iy+obj_proc.status), PROC_RUNNING
    di
    ld hl, 0xfd9a
    ld de, proc_switch.hkeyi
    ld bc, 5
    ldir
    ld hl, proc_hkeyi
    ld de, 0xfd9a
    ld bc, 5
    ldir
    ei
    ret



proc_hkeyi:
    jp proc_switch
    ret
    ret

; MSX 1/2 KEYINT STACK (DIFFERS ON MSX TURBO R)
; interrupt return
; hl
; de
; bc
; af
; hl
; de
; bc
; af
; iy
; ix
; RET FROM CALL H.KEYI

proc_switch:
    ld ix, (_curr_proc)
    ld hl, 0
    add hl, sp
    ld (ix+obj_proc.stack_top), l
    ld (ix+obj_proc.stack_top+1), h
    ld e, (ix+obj_proc.next_ptr)
    ld d, (ix+obj_proc.next_ptr+1)
    ld ix, 0
    add ix, de
    ld l, (ix+obj_proc.stack_top)
    ld h, (ix+obj_proc.stack_top+1)
    ld sp, hl
    .hkeyi:
    ret
    ret
    ret
    ret
    ret
    ret


