; Sockets interface

; Constants used for arguments(from kernel source)
AF_INET				equ 2
IPPROTO_TCP			equ 6
SOCK_STREAM			equ 1
PF_INET				equ 2
MSG_PEEK			equ 2
MSG_WAITALL			equ 100h
SOL_SOCKET			equ 1
SO_REUSEADDR		equ 2

; ------------------------------------------------------------

; Function numbers
SYS_SOCKET			equ 1
SYS_BIND			equ 2
SYS_LISTEN			equ 4
SYS_ACCEPT			equ 5
SYS_SEND			equ 9
SYS_RECV			equ 10
SYS_SETSOCKOPT		equ 14

; Step increase memory for receive buffer
REALLOC_STEP		equ 64

; ------------------------------------------------------------

; begin recv_args structure (argument for recv function)
recv_args			equ 8						; main offset from esp
recv_args_sock		equ esp + recv_args
recv_args_buffer	equ esp + 4 + recv_args
recv_args_length	equ esp + 8 + recv_args
recv_args_flags		equ esp + 12 + recv_args
send_args			equ 16
send_args_sock		equ 16

; ------------------------------------------------------------

sock:
		;mov ebp, esp

	;int socket(int domain, int type, int protocol);
	.doSocket:
		mov		ebx, SYS_SOCKET					; socket() = int call 1
		mov		ecx, dword sock_args			; arguments to socket()
		call	sys_sock
		mov		[socketfd], eax					; save socket handle
		cmp		eax, -1
		je		sockerr

	;int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
	.setSockOpt:
		; ignoring port in state TIME_WAIT
		mov		ebx, SYS_SETSOCKOPT
		mov		eax, dword [socketfd]
		mov		[sockopts_args.sockfd], eax
		mov		ecx, dword sockopts_args
		call	sys_sock
		cmp		eax, -1
		je		sockerr

	;int bind(int socket, const struct sockaddr *address, socklen_t address_len);
	.doBind:
		; fillup sockaddr
		mov		[sa.sin_family], word AF_INET	; address family
		mov		al, byte [cfgBindPort+1]		; reverse bytes hi=>lo
		mov		ah, byte [cfgBindPort]			; reverse bytes lo=>hi
		mov		[sa.sin_port], ax				; set port
		mov		eax, dword [cfgBindIp]			; get ip from config
		mov		[sa.sin_addr], eax				; set ip to structure
		; form arguments
		mov		eax, dword [socketfd]			; copy socket
		mov		[bind_args.fd], eax				; handle to struct
		mov		ebx, SYS_BIND					; require bind()
		mov		ecx, bind_args					; bind arguments
		call	sys_sock
		cmp		eax, -1
		je		binderr
		
	; int listen(int socket, int backlog);
	.doListen:
		mov		eax, [socketfd]					; copy socket handle
		mov		[listen_args.sock], eax			; to structure listen_args
		mov		ebx, SYS_LISTEN					; require listen()
		mov		ecx, listen_args				; arguments
		call	sys_sock
		cmp		eax, -1
		je		listenerr

	;accept(fd, NULL, 0);
	.doAccept:
		mov		eax, [socketfd]					; copy socket handle
		mov		[accept_args.sockfd], eax		; to structure
		mov		ebx, SYS_ACCEPT					; we need accept() function
		mov		ecx, accept_args				; ptr to arguments
		call	sys_sock
		cmp		eax, -1
		je		accepterr

	.processConnection:
	; here we have a connected socket peer in eax
		push	eax
		call	connectionHandler
		jmp		.doAccept

; ------------------------------------------------------------
	
;exit_s(0)
exit_s:
	;mov esp, ebp
    ret 

; ------------------------------------------------------------

; These are just to print an error message to stdout should
; one of our socketcall()'s fail.
sockerr:
		cinvoke printf, socksbroke
		jmp		exit_s

binderr:
		cinvoke printf, bindsbroke
		jmp		exit_s

listenerr:
		cinvoke printf, listensbroke
		jmp		exit_s

accepterr:
		cinvoke printf, acceptsbroke
		jmp		exit_s

; ------------------------------------------------------------
tsock	equ dword [ebp+8]  ; connected socket

; connection handler function
; get 1 arg - socket handle
connectionHandler:
		push	ebp
		mov		ebp, esp						; preserve esp
		jmp		.initVars

	.vars:
	; declare variables (allocate memory for each)
		sub		esp, 16							; allocate memory for recv_args

	.initVars:
		 mov	eax, tsock
		 mov	[recv_args_sock], eax
		 mov	[recv_args_flags], dword MSG_PEEK
		 mov	[recv_args_length], dword REALLOC_STEP

	.code:
	.alloc:
		cinvoke malloc, [recv_args_length]
		mov		[recv_args_buffer], eax			; save pointer to buffer

	.recvLength:
		mov		ebx, SYS_RECV					; we need recv()
		mov		ecx, esp
		add		ecx, recv_args					; arguments
		call	sys_sock
		cmp		eax, dword -11					; try again code?
		je		.recvLength						; recieve again

		mov		edx, eax
		mov		ebx, dword [recv_args_length]
		cinvoke printf, retval,ebx,edx
		pop		eax
		cmp		eax, [recv_args_length]
		jb		.recv

	.realoc:
		add		[recv_args_length], dword REALLOC_STEP
		mov		eax, dword [recv_args_buffer]
		cinvoke realloc, eax,[recv_args_buffer]
		mov		[recv_args_buffer], eax
		jmp		.recvLength

	.recv:
		mov		ebx, SYS_RECV					; we need recv()
		mov		[recv_args_flags], dword 0		; without flags!
		mov		ecx, esp						; point ecx ..
		add		ecx, recv_args					; .. to arguments
		call	sys_sock						; do recv()
		mov		[recv_args_length], eax			; store received message length
		cmp		eax, 0							; is there any error?
		jbe		.closeSocket					; then cancel request

		pushd	[recv_args_buffer]
		pushd	tsock
		call	processRequest					; let's process request!

	.closeSocket:
		push	tsock
		call	sys_close

	.freeMem:
		mov		eax, dword [recv_args_buffer]
		cinvoke free, eax
		mov		[recv_args_length], dword 0		; mark block as free

	.return:
		cmp		[recv_args_length], dword 0		; we used malloc?
		jne		.freeMem						; yeah, so we need free memory

		mov		esp, ebp						; restore stack poiter
		pop		ebp
		ret		4

; ------------------------------------------------------------

; send to socket 'Server' header
; first argument in stack - sock handle
sendServerHeader:
		push	ebp
		mov		ebp, esp
		cinvoke strlen, headerServer
		push	dword 0							; flags
		push	eax								; length
		push	dword headerServer				; header string
		push	dword [ebp + 8]					; socket handle
		mov		ecx, esp						; ptr to struct in stack
		mov		ebx, 9							; send() function number
		call	sys_sock						; call it
		add		esp, 4 * 4						; correct stack space

		mov		esp, ebp						; restore base stack pointer
		pop		ebp         
		ret		4

; vim: ft=fasm

