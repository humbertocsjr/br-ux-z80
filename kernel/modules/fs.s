obj_inode: equ 16
    .id: equ 0
    .process_id: equ 2
    .read_handler: equ 4
    .write_handler: equ 6
    .seek_handler: equ 8
    .close_handler: equ 10
    .position: equ 12
    .sub_position: equ 14
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