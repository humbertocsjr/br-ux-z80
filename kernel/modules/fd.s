_fd0_inode_id:
    db 0
_fd1_inode_id:
    db 0

; de = inode
; hl = source
; bc = len
fd_write:
    scf
    ccf
    ret
    