sys_exit:   ; System call for exit
    mov eax, 1      ; Function number for exit program
    mov ebx, 0      ; Return code - it's okay (0)
    int 80h         ; Do kernel call
    ret

sys_fork:      ; System call for fork()
    mov eax, 2
    int 80h
    ret

sys_getpid:    ; System call to get current PID
    mov eax, 20     ; sys_getpid
    int 80h
    ret

sys_sock: ; Socket functions
    mov eax, 102       ; socketcall() syscall
    int 0x80           ; Call the kernel
    ret

sys_close:
    mov eax, 6                   ; close() syscall
    mov ebx, dword [esp+4]
    int 0x80                   ; Call the kernel
    ret 4

; vim: ft=fasm

