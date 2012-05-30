; helper macro for definition of relative structure offsets
macro struct name
{
	virtual at -30
		name name
	end virtual
}

; virtual at ebp - 30 ; base offset = -30
struc request_t {
  .filename    dd    ? 
  .method      dd    ?
  .socket      dd    ?
  .request     dd    ?
  .requestLen  dd    ?
}
struct request_t

lv_offset	equ		30
headers		equ		ebp - 30 - lv_offset + 0
i			equ		ebp - 30 - lv_offset + 4
j			equ		ebp	- 30 - lv_offset + 8
buf			equ		ebp - 30 - lv_offset + 12
len			equ		ebp	- 30 - lv_offset + 16
sstr		equ		ebp - 30 - lv_offset + 20

; ------------------------------------------------------------

; Process user requests
; arguments: socket/(dword) request(dword,ptr)
processRequest:
		push	ebp									; preserve ebp
		mov		ebp, esp							; new local offset
		sub		esp, 30								; space for structure in stack
	    sub		esp, lv_offset
		mov		eax, dword [ebp + 8]				; get socket id from stack
		mov		[ebp + request_t.socket], eax
		mov		eax, dword [ebp + 12]				; get request string
		mov		[ebp + request_t.request], eax
		cinvoke	printf, debug_msg
		cinvoke strlen,	[ebp + request_t.request]
		mov		[ebp + request_t.requestLen], eax	; remember length
		cinvoke printf, debug_msg
		pushd	0
		push	'->> '
		mov		eax, esp
	    pushd	0
		push	'<<- '
		cinvoke printf, strDebugGeneral,eax,esp
    
    .allocBuffers:
		;.. for header line
		; requre 16Kb of memory
		cinvoke malloc, (1 shl 14)
		mov		[buf], eax							; save pointer to memory block
		mov		[eax], dword 0						; length = 0
		; ..for array of pointers to string(enviroment)
		mov		eax, dword [maxHeaders]
		mov		bl, 4
		mul		bl
		cinvoke malloc, eax
		mov		[headers], eax
		cinvoke memset, [headers], 0
		; ..for url buffer
		; 2048 - maximum url length (rfc)
		cinvoke malloc, 2048
		mov		[ebp + request_t.filename], eax		; save point to url

	.breakHeaders:
		cinvoke strtok, [request_t.request],header_sep
		cmp		eax, 0
		je		.breakHeadersEnd

	.httpRequest:
		mov		[sstr], eax							; save pointer to header string
		mov		ebx, [sstr]
		mov		[ebp + request_t.method], ebx
		cinvoke strchr, [sstr],' '
		mov		[eax], byte 0						; break method and url
		inc		eax
		mov		[ebp + request_t.filename], eax
		cinvoke strchr, [ebp + request_t.filename],' '
		mov		[eax], byte 0						; break url and http
		cinvoke strtok, 0,header_sep
		cmp		eax, 0
		je		.breakHeadersEnd

	.breakHeadersLoop:
		mov		[sstr], eax							; save pointer to header string
		;pushd	[sstr]								; get current header length
		cinvoke printf, juststr, [sstr]
		;pop		eax
		pop		eax
		;jmp .breakHeadersEnd
		cinvoke strtok, 0,header_sep
		cmp		eax, 0
		jne		.breakHeadersLoop

	.breakHeadersEnd:
		cinvoke printf, juststr,[ebp + request_t.filename]
		mov		eax, ebp
		;add	eax, request_t
		sub		eax, 30
		push	eax
		call	preprocessRequest

	.endproc:
	.freeBuffers:
		cinvoke free, [headers]
		cinvoke free, [buf]
		mov		esp, ebp							; restore base stack pointer
		pop		ebp
		ret		4 * 2
 
; ------------------------------------------------------------
   
preprocessRequest:
		push	ebp									; preserve ebp
		mov		ebp, esp							; new local offset

		reqFilename equ ebp+16
		;cinvoke printf, debug_msg

		pushd	0
		pushd	10									; count of bytes
		pushd	[reqFilename]
		pushd	[ebp+8]
		pop		eax
		add		eax, 8
		push	eax
		call	send
		add		esp, 4*4
		; get '/cgi-bin'
		cinvoke strlen, cgiBinFolder
		; compare length is a last argument
		; compare start of strings
		cinvoke strncmp, [reqFilename],cgiBinFolder,eax
		cmp		eax, 0								; is start are same in both strings?
		jz		.cgiHere							; hey! there is cgi program!

	.staticHere:
		mov		ebx, ebp							; filename - first field in structure
		add		ebx, 16
		pushd	[ebx]
		call	handlerStatic						; call static content processor
		jmp		.endproc

	.cgiHere:
		jmp		.cgiHereNext

	.cgiherestring:
		db 'Hey! There is cgi interface!', 10, 0

	.cgiHereNext:
		cinvoke printf, .cgiherestring
		jmp		.endproc

	.endproc:
		mov		esp, ebp							; restore base stack pointer
		pop		ebp
		ret		4

; vim: ft=fasm

