; Exercicio Programa 1 - MAC0216 (2023)
; Laufernando Souza Dias - 11222947

; estrutura (ordem) do EP: 
	; leitura com syscall read e seu tamanho
	; passos 1, 2, 3 e 4
	; sub-rotinas auxiliares
	; syscalls write e exit
	; section .data
	; section. bss

global _start

section .text
_start:
	mov rax, 0 ; sys_read para ler a entrada x 
	mov rdi, STDIN
	mov rsi, x
	mov rdx, 100100
	syscall

	dec eax    ; armazena o tamanho da entrada x 
	mov dword [tamanho], eax

; ----------------------------------------------

passo1: 
	call limpaRaxRdx         ; sub-rotina que limpa rax e rdx
	
	mov ax, word [tamanho]   ; divisao do tamanho por 16
	mov dx, word [tamanho+2]
	mov cx, 0x10
	div cx

	or dx, dx   		 ; verifica se resto==0: se for pular, 
			    	 ; se nao, incrementa em ax...
	jz atribuicao
	inc ax
	atribuicao: 		 ; ...e na sequencia armazena o 
		mov word [n], ax ; numero de blocos
	
	or dx, dx   		 ; se resto==0: se for, pula pro passo 2
	jz passo2
	
	call limpaRbx 		 ; sub-rotinas que limpam rbx e rcx
	call limpaRcx

	neg dx
	add dx, 0x10 
	mov bx, dx 		 ; bx recebe (16 - resto)
	add edx, dword [tamanho] ; edx recebe tamanho + (16 - resto)
	mov ecx, dword [tamanho] ; ecx recebe o tamanho 
				 ; p/ controlar o loop que preenche
				 ; o bloco com o valor em bx

	mov dword [tamanho], edx ; atualiza o tamanho do bloco

	call preencherPosicoes   ; chama o loop para ajustar 
				; o tamanho do ultimo bloco

passo2:
	call limpaRaxRdx
	call limpaRbx
	call limpaRcx
	
	loopExterno: 		 ; loop com n iteracoes e um 
				 ; interno c/ 16 iteracoes
			         ; cx controla o externo, dx o interno
				 ; ax contem o novoValor
				 ; ebx vai armazenando o valor (cx * 16 + dx)
		mov dx, 0
		xor r8, r8 	 ; limpa o r8 que sera usado apenas aqui
		call loopInterno

		inc cx
		cmp cx, word [n] ; comparacao iterador
		jl loopExterno 

	call concatenarBloco     ; saidaPassoDois nada mais eh do que
				 ; o array x com esse bloco concatenado

passo3:
	call limpaRcx 		 ; rcx equivale ao i no cod. python

	loopPrincipal:
		call limpaRaxRdx ; rdx equivale ao j no cod. python
				 ; rax auxilia para trocar dados 
				 ; de memoria p/ reg. e vice-versa

		shl ecx, 4       ; equivale a multiplicar por 16 para usar
				         ; ecx * 16 no indice do vetorMagico
		call loop16      ; chama o loop com 16 iteracoes
		shr ecx, 4       ; traz ecx pro valor correto de volta

		call limpaRaxRdx ; rax sera o temp
		call loop18 	 ; chama o loop com 18 iteracoes

		inc cx
		cmp cx, word [n] ; comparacao iterador
		jl loopPrincipal

passo4:
	call limpaRaxRdx 					 ; ax pega os valores em saidaPassoTres
					 			 ; dl auxilia p/ transferir resto e quoc.
	call limpaRcx    					 ; cx contador
	mov rbx, 0x10	 					 ; bl vai fazer divisoes por 16

	montaHash:
		mov ax, word [saidaPassoTres+ecx]; conversao dos 16 bytes
								 ; em caracteres (hexadecimal)

		; exemplo: 4f dividido por 16 gera quociente 4 e resto f, facilitando
		; atribuir cada byte na saidaPassoTres a dois caracteres do vetor
		; caracteres (ver section .data)

		and ax, 0xff 					 ; limpa a parte ah do ax
		div bl 		 				 ; quociente em al, resto em ah
		
		mov dl, al 
		mov dl , [caracteres+edx]
		mov byte [hash+ecx+ecx], dl 	                 ; ecx*2 pois sao 2 caracteres
								 ; para 1 hexadecimal

		mov dl, ah
		mov dl , [caracteres+edx]
		mov byte [hash+ecx+ecx+1], dl   		 ; ecx*2 pois sao 2 caracteres
							         ; para 1 hexadecimal

		inc cx
		cmp cx, 0x10
		jl montaHash					 ; comparacao iterador

	jmp imprimirSaida

; ----------------------------------------------

limpaRaxRdx:
	xor rax, rax ; zera rax e rdx, os quais sao
	xor rdx, rdx ; requisitados muitas vezes
	ret

limpaRbx:
	xor rbx, rbx ; zera rbx
	ret

limpaRcx:
	xor rcx, rcx ; zera rcx
	ret

preencherPosicoes: 
	mov [x+ecx], bl ; o que esta em bl basta,
			; pois eh 1 byte apenas
	inc cx
	cmp cx, dx      ; comparacao iterador
	jl preencherPosicoes

	ret

loopInterno:
	mov r8b, byte [x+ebx] 		   ; r8b recebe saidaPassoUm[cx * 16 + dx]
	xor rax, r8 		  	   ; primeiro xor
	and rax, 0xff 		  	   ; limpa rax (mantem o que esta em al)

	mov al, byte [vetorMagico+eax]     ; usa como indice no vetorMagico
	xor al, byte [novoBloco+edx]       ; segundo xor
	mov byte [novoBloco+edx], al

	inc rbx

	inc dx
	cmp dx, 0x10			   ; comparacao iterador
	jl loopInterno
	ret

concatenarBloco:
	call limpaRaxRdx
	call limpaRcx

	mov eax, dword [tamanho]             ; eax para caminhar no array x
	loopConcatena:
		mov dl, byte [novoBloco+ecx] ; dl auxilia mover de
	   				     ; um byte na memoria para outro
		mov byte [x+eax], dl         ; e assim concatenar o bloco

		inc rax
		inc rcx
		cmp rcx, 0x10		     ; comparacao iterador
		jl loopConcatena

	mov ax, word [n] 	             ; atualiza o valor de n para
	inc ax				     ; o prox. passo
	mov word [n], ax 
					 
	ret

loop16:
	mov al, byte [x+ecx+edx] 	     ; usa a saida do passo 2
	mov byte [saidaPassoTres+edx+16], al 
	xor al, byte [saidaPassoTres+edx]    ; faz xor entre dois
					     ; elementos da saidaPassoTres
	mov byte [saidaPassoTres+edx+32], al ; armazena na saidaPassoTres

	inc dx
	cmp dx, 0x10 			     ; comparacao iterador
	jl loop16
	ret

loop18:
	call limpaRbx		 ; rbx equivale ao k no cod. python
	call loop48  		 ; chama o loop com 48 iteracoes
	add ax, dx	         ; ax guarda temp, dx guarda j, entao...
	cmp ax, 0x100		 ; ... calcula (temp + j) % 256 sem precisar 
				 ; dividir pois 0=<temp<256 e 0<=j<18
			         ; se (temp + j) < 256, mantem a soma
		                 ; caso contrario, temp = (ax + dx) - 256
	jl continuacao
	sub ax, 0x100

	continuacao:
	inc dx
	cmp dx, 0x12	         ; comparacao iterador
	jl loop18
	ret

loop48:
	mov al, byte [vetorMagico+eax]    ; acessa vetorMagico[temp]
	xor al, byte [saidaPassoTres+ebx] ; faz o xor com um elemento
	mov byte [saidaPassoTres+ebx], al ; de saidaPassoTres

	inc bx
	cmp bx, 0x30 			  ; comparacao iterador
	jl loop48
	ret

; ----------------------------------------------

imprimirSaida:
	; sys_write para imprimir o hash (saida)
	mov rax, 1
	mov rdi, STDOUT 
	mov rsi, hash
	mov rdx, 33
	syscall

finalizarPrograma:
    ; sys_exit para sair do programa
	mov rax, 60
	mov rdi, 0
	syscall

; ----------------------------------------------

section .data
	; atalhos para deixar o codigo mais legivel
	STDIN: equ 0
	STDOUT: equ 1
	ENTER: equ 0x0a

	; inicializa o tamanho da string
	tamanho: dd 0

	; reserva espaço para guardar o num. de blocos de 16 bytes
	n: dw 0

	; define o vetor magico
	vetorMagico: db 122, 77, 153, 59, 173, 107, 19, 104, 123, 183, 75, 10
	db 114, 236, 106, 83, 117, 16, 189, 211, 51, 231, 143, 118, 248, 148, 218
db 245, 24, 61, 66, 73, 205, 185, 134, 215, 35, 213, 41, 0, 174, 240, 177,
db 195, 193, 39, 50, 138, 161, 151, 89, 38, 176, 45, 42, 27, 159, 225, 36,
db 64, 133, 168, 22, 247, 52, 216, 142, 100, 207, 234, 125, 229, 175, 79,
db 220, 156, 91, 110, 30, 147, 95, 191, 96, 78, 34, 251, 255, 181, 33, 221,
db 139, 119, 197, 63, 40, 121, 204, 4, 246, 109, 88, 146, 102, 235, 223,
db 214, 92, 224, 242, 170, 243, 154, 101, 239, 190, 15, 249, 203, 162, 164,
db 199, 113, 179, 8, 90, 141, 62, 171, 232, 163, 26, 67, 167, 222, 86, 87,
db 71, 11, 226, 165, 209, 144, 94, 20, 219, 53, 49, 21, 160, 115, 145, 17,
db 187, 244, 13, 29, 25, 57, 217, 194, 74, 200, 23, 182, 238, 128, 103,
db 140, 56, 252, 12, 135, 178, 152, 84, 111, 126, 47, 132, 99, 105, 237,
db 186, 37, 130, 72, 210, 157, 184, 3, 1, 44, 69, 172, 65, 7, 198, 206,
db 212, 166, 98, 192, 28, 5, 155, 136, 241, 208, 131, 124, 80, 116, 127,
db 202, 201, 58, 149, 108, 97, 60, 48, 14, 93, 81, 158, 137, 2, 227, 253,
db 68, 43, 120, 228, 169, 112, 54, 250, 129, 46, 188, 196, 85, 150, 6, 254,
db 180, 233, 230, 31, 76, 55, 18, 9, 32, 82, 70 

	; inicializa novo bloco (passo 2)
	novoBloco: db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 

	; inicializa 3 blocos (passo 3)
	saidaPassoTres: db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
	; inicializa variavel temp (passo 3)
	temp: db 1

	; inicializa a saida hash com ENTER no final
	hash: db 0, 0, 0, 0, 0, 0, 0, 0 
db 0, 0, 0, 0, 0, 0, 0, 0 
db 0, 0, 0, 0, 0, 0, 0, 0 
db 0, 0, 0, 0, 0, 0, 0, 0, 0x0a

	; define caracteres para montar a saida 
	caracteres: db "0123456789abcdef"

; ----------------------------------------------

section .bss
	x: resb 100016 ; buffer para os bytes da string x
		       ; mais um espaco para novoBloco
