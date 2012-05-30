format ELF executable 3
entry start

include 'import32.inc'
include 'proc32.inc'

interpreter '/lib/ld-linux.so.2'
needed 'libc.so.6'
import printf,fopen,fclose,feof,fseek,ftell,fread,fgets,malloc,realloc,memset,free,strlen,strtok,strchr,strcpy,strncmp,strcmp,send,atoi,inet_addr,fork

segment readable executable

start:

	jmp main ; main()
	
include 'syscall.inc'
include 'sockets.inc'
include 'requests.inc'
include 'static.inc'

buffLen    equ 1024

; ====================== Main body ========================
main:
		mov		eax, dword [isDoFork]		; should we do fork() ?
		cmp		eax, 0						; really?
		jz		.run						; no, just continue loading

	.fork:
		call	sys_fork					; do fork()
		cmp		eax, 0						; 0 means we are forked
		je		.run						; run the server
		jmp		.exit						; parent process should exit

	.run:
		call	load_conf					; load configuration from file
		cinvoke printf, strStartMsg,[cfgBindIp],[cfgBindPort]
		call	sock						; create and listen network socket

	.exit:
		cinvoke printf, exit_msg
		call	sys_exit					; stop the process

; ================== Loading config file ===================

; procedure load and parse configuration data
load_conf:
		; open config file in read-only mode
		cinvoke fopen, fileConfig,fOpenRead
		mov		[var.fHandle], eax
		cmp		eax, 0
		jne		.readFile					; eax != 0 - that's ok!

	.cantOpen:
		; Output error message
		cinvoke printf, strFileNotFound,fileConfig
		cinvoke printf, strTerminating
		call	sys_exit					; quit...
		jmp		.endProc

	.readFile:
	.checkEof:  
		; Check for end of file
		cinvoke feof, [var.fHandle]
		cmp		eax, 0
		jnz		.closeFile					; EOF?!

	.getLine:
		; get 1 line from file
		cinvoke fgets, buff,buffLen,[var.fHandle]
		test	eax, eax					; check for error
		jz		.readFile					; eax = 0 means error

	.correctString:
		; Correct end of string - where '#' located
		cinvoke strchr, buff,'#'
		cmp		eax, 0						; eax == 0 ?
		je		.checkLength				; so check length next

		mov		[eax], byte 0				; set end of string

	.checkLength:
		; check for zero length
		cinvoke strlen, buff
		test	eax, eax					; what about eax?
		jz		.readFile					; It's empty string!?

	.removeChr10:
		; remove end-of string symbol
		cinvoke strchr, buff,10
		test	eax, eax
		jz		.isKeyValuePair
		mov		[eax], byte 0

	.isKeyValuePair:
		; Search for '=', which split name = value" pair
		cinvoke strchr, buff,'='
		test	eax, eax					; eax?!
		jz		.readFile					; if eax == 0, read next line

	.split:
		; Splits pair
		cinvoke strtok, buff,var.delimiter
		mov		[var.key], eax				; key string
		;strtok(NULL, delimeter)
		cinvoke strtok, 0,var.delimiter
		mov		[var.value], eax			; value string

	.checkForPort:
		; is key == port?
		cinvoke strcmp, var.kPort,[var.key]
		test	eax, eax					; what about eax?
		jne		.checkForIp					; not equals? next check
		cinvoke atoi, [var.value]
		mov		[cfgBindPort], ax			; save value
		jmp		.checkEnd					; we are done

	.checkForIp:
		; is key == ip?
		cinvoke strcmp, var.kIp,[var.key]
		test	eax, eax					; what about eax?
		jne		.checkForRoot				; not equals? next check
		
		cinvoke	inet_addr, [var.value]		; do transform
		mov		[cfgBindIp], eax			; save value

	.checkForRoot:
		; is key == root?
		cinvoke strcmp, var.kRoot,[var.key]
		test	eax, eax					; what about eax?
		jne		.checkEnd					; not equals? next check
		
		; copy config value item
		cinvoke strcpy, cfgRoot,[var.value]

	.checkEnd:
		jmp		.readFile
		cinvoke printf, var.pair,[var.key],[var.value]
		jmp		.readFile					; read next line

	.closeFile:
		cinvoke fclose, [var.fHandle]

	.endProc:
		ret

; ------------------------------------------------------------

segment readable

fOpenRead:     db 'r', 0 ; read only flag 
fOpenWrite:    db 'w', 0 ; write flag 
fOpenAppend:   db 'a', 0 ; append file
strFileNotFound:  db 'File "%s" is not found!', 10, 0
strTerminating:   db 'Terminating application..', 10, 0
strDebugGeneral:  db '%s: %s', 10, 0
strDebugHex:	  db '[0x%08x]', 10, 0
strStartMsg:      db 'Starting up server, build date: 25.05.2012 23:00 Bind to %ld:%d', 10, 13, 0
strProcessForked: db 'Process forked, pid: %d', 10,  0
strHandlerStatic: db 'handlerStatic: %s', 10, 0
fileDebug:      db './debug.log',0  ; file for debug info
fileConfig:     db './aweb.conf',0  ; config file
maxConnections: dd 1024
maxHeaders: dd 1024
cgiBinFolder: db '/cgi-bin', 0  ; prefix for cgi programs
headerHTTP200: db 'HTTP/1.0 200 OK', 10, 13, 0
headerServer: db "Server: FASM web server with CGI support", 10, 13, 0 
headerStd: 
    db 'Connection: close', 10, 13
    db 'Cache-Control: no-cache,no-store,max-age=0,must-revalidate', 10, 13
    db 10, 13
    db 0
isDoFork: dd 0

segment writeable

cfgBindIp:     dd ((127 shl 0) or (0 shl 8) or (0 shl 16) or (1 shl 24))
cfgBindPort:   dd 808
cfgRoot:       db '/home/xvilka/ASM/Web2'
    times 120  db 0

header_sep:      db 10, 13, 0
juststr:         db "%s",10,0

connect			db "We have a connection!", 10, 0               ; First string...
hello			db "Hello... =]", 10, 0                           ; Second string...
goodbye			db "Goodbye... =[", 0                           ; Third string...
socksbroke		db "ERROR: socket() failed!", 10, 0           ; Fourth string...
bindsbroke		db "ERROR: bind() failed!", 10, 0               ; Fifth string...
listensbroke    db "ERROR: listen() failed!", 10, 0           ; Sixth string...
acceptsbroke    db "ERROR: accept() failed!", 10, 0           ; Seventh string...
retval          db "value buffer: %d, recv: %d", 10, 0
retstr          db "received: %s", 10 , 0
html            db "HTTP 1.1", 10, 0, '$'
html_len        equ $ - html
one             dd 1

exit_msg		db "Exiting...",10,0
debug_msg		db "is key pair?",10,0

sa: ; sockaddr_in structure
.sin_family dw    0
.sin_port   dw    0
.sin_addr   dd    0

sock_args: ; socket() function argument
    dd PF_INET, SOCK_STREAM, IPPROTO_TCP


bind_args:						; bind() function arguments
    .fd         dd 0				; socket handle
    .sockaddr   dd sa				; pointer to sockaddr structure
    .socklen_t  dd 16				; socklen_t addrlen

listen_args:					; listen() function arguments
    .sock       dd 0				; sock handle
    .backlog    dd maxConnections	; max connections
    
accept_args:					; accept() arguments   
    .sockfd     dd 0				; socket handle
    .addr       dd 0				; struct sockaddr *addr
    .addrlen    dd 0				; socklen_t *addrlen
    
sockopts_args:					; setsockopt() and getsockopt() args
    .sockfd     dd 0				; socket handle
    .level      dd SOL_SOCKET		; manipulation level
    .optname    dd SO_REUSEADDR		; option in selected level
    .optval     dd one				; option value
    .optlen     dd $ - one			; size of option value
    
socketfd: dd 0					; socket handle

var:
.fHandle    dd 0
.num        db 'num = %d', 10, 0
.pair       db '|%s = %s|', 10, 0
.hint       db ' hint = "%s"', 10, 0
.hintd      db ' hint = "%d"', 10, 0
.delimiter  db '= ', 0
.key        dd 0
.value      dd 0
.kPort      db 'port', 0
.kIp        db 'ip', 0
.kRoot      db 'root', 0

buff:      db buffLen dup (?)

; vim: ft=fasm

