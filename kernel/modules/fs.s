obj_inode: equ 10
    .id: equ 0
    .process_id: equ 2
    .device_id: equ 4
    .position: equ 6
    .sub_position: equ 8
qty_inode: equ 64

obj_dirent: equ 68
    .id: equ 0
    .name: equ 64
    .zero: equ 66

_inodes: 
    times obj_inode * qty_inode db 0

fs_init:
    ret

fs_open:
    ret

fs_mknod:
    ret