; File informational struct
filename		equ ebp + 8
fullfilename	equ ebp - 4
filehandle		equ ebp - 8
filelength		equ ebp - 12

; ------------------------------------------------------------

handlerStatic:

		push	ebp							; preserve ebp
		mov		ebp, esp					; new local offset   
		sub		esp, 4						; for filename pointer
		mov		dword [fullfilename], 0
		sub		esp, 4						; for filename pointer
		mov		dword [filehandle], 0
		sub		esp, 4						; for filename pointer
		mov		dword [filelength], 0
 
		cinvoke strlen, cfgRoot
		push	eax
		cinvoke strlen, [filename]
		pop		edx
		add		eax, edx					; in eax - length of two strings(for full path)
		inc		eax							; eax++ for terminating '0'
		; get some memory from system
		cinvoke malloc, eax
		mov		[fullfilename], eax			; save pointer
		test	eax, eax					; what about zero?
		jz		.endproc					; zero pointer?
		
		cinvoke strcpy, [fullfilename],cfgRoot
		cinvoke strlen, cfgRoot
		add		eax, [fullfilename]			; calculate end of string
		cinvoke strcpy, eax,[filename]
		cinvoke printf, strHandlerStatic,[fullfilename]
    
	.fileOpen:
		cinvoke fopen, [fullfilename],fOpenRead
		mov		[filehandle], eax			; save file handler
		cmp		eax, 0						; check for error
		jz		.notfound					; error(=not found)?

	.fileSize:
		; move pointer to the end
		cinvoke fseek, [filehandle],0,2
		cinvoke ftell, [filehandle]			; where is a file pointer?
		mov		[filelength], eax			; save file length
		cmp		eax, 0						; empty file?
		jz		.notfound					; so, it's like no file

	.allocMemoryForFile:
		cinvoke malloc,[filelength]
		cmp		eax, 0
		jz		.closeFile
		push	eax							; save memory pointer
	
	.readFile:
		; move pointer to begin
		cinvoke fseek, [filehandle],0,0
		pop		eax
		push	eax
		cinvoke fread, eax,1,[filelength],[filehandle]
    
	.writeToSocket:
		; first of all, send HTTP 200 answer
		cinvoke strlen, headerHTTP200
		push	dword 0						; flags
		push	eax							; length
		push	dword headerHTTP200			; header string
		mov		ebx, dword [ebp+20]			; start of structure in processRequest
		push	dword [ebx+8]				; socket handle
		mov		ecx, esp					; ptr to struct in stack
		mov		ebx, 9						; send() function number
		call	sys_sock					; call it
		add		esp, 4 * 4					; correct stack space
		; ..and server info
		mov		ebx, dword [ebp+20]			; start of structure in processRequest
		push	dword [ebx+8]				; socket handle is argument (from struct)
		call	sendServerHeader			; call it
		; next, send generic headers
		cinvoke strlen, headerStd
		push	dword 0						; flags
		push	eax							; length
		push	dword headerStd				; header string
		mov		ebx, dword [ebp+20]			; start of structure in processRequest
		push	dword [ebx+8]				; socket handle
		mov		ecx, esp					; ptr to struct in stack
		mov		ebx, 9						; send() function number
		call	sys_sock					; call it
		add		esp, 4 * 4					; correct stack space
		pop		edx							; addr of buff
		push	edx							; and save it again
		mov		ebx, dword [ebp+20]			; start of structure in processRequest
		; at last, send file contents
		push	dword 0
		push	dword [filelength]			; count of bytes
		push	dword edx					; buffer to send
		push	dword [ebx+8]				; socket handle - offset in structure
		mov		ecx, esp					; ptr to struct in stack
		mov		ebx, 9						; send() function number
		call	sys_sock					; call it
		add		esp, 4 * 4					; correct stack space
		; first arg in eax(buffer)
		cinvoke printf, strHandlerStatic

	.freeBuffer:
		call	free						; release memory(pointer in head of heap)
		pop		eax							; stack correct (was addr of memory in heap)

	.closeFile:
		cinvoke fclose, [filehandle]
		jmp		.endproc					; go to end

	.notfound:
		jmp		.endproc

	.endproc:
	.freeMem:
		cmp		dword [fullfilename], 0		; memory allocated?
		je		.correctStack				; no? go away
		cinvoke free, [fullfilename]

	.correctStack:
		mov		esp, ebp					; restore base stack pointer
		pop		ebp
		ret		4

; vim: ft=fasm

