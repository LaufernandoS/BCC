AUTOR:
Laufernando Souza Dias, 11222947, laufernando@usp.br

DESCRIC ̧ ̃AO:

Os programas em python e em assembly leem uma entrada de texto 
(pode ser convertida atraves de um arquivo .txt) com um texto, e transformam 
em um codigo hash seguindo uma mesma sequencia de passos. A sequencia de 
passos consiste na leitura da entrada, no ajuste dos bytes da entrada para 
um tamanho multiplo de 16, na concatenacao de um bloco extra de bytes, na 
reducao a tres blocos de 16 bytes e no uso dos primeiros 16 bytes desses 
blocos para a saida em hexadecimal, numa sequencia de operacoes logicas (xor) 
que dificultam a transformacao de textos distintos num mesmo hash.

Essas implementacoes sao parte do exercicio programa da disciplina
MAC0216 (2023) - Tecnicas de Programacao I do
Bacharelado em Ciencia da Computacao (IME-USP). Para mais informacoes
a respeito dos tempos de execucao e afins, ver o relatorio.
____________________________________________________________________________

COMO GERAR O EXECUT ́AVEL:

Acesse o diretorio em que se encontra os arquivos utilizados.
Para gerar o executavel do programa em Assembly, segue a sequencia de
comandos:

nasm -f elf64 ep1.s -o ep1.o
ld -s -o ep1 ep1.o
____________________________________________________________________________

COMO EXECUTAR:

Para executar o programa em Python, digitar o comando no terminal:

$ python ep1.py

ou...

$ python3 ep1.py

ou... (com a entrada .txt como argumento):

$ python ep1.py < <arquivo.txt> 
$ python3 ep1.py < <arquivo.txt> 


Para executar o programa em Assembly, digitar o comando no terminal:

$ ./ep1

ou... (com redirecionamento da entrada .txt):

$ ./ep1 <arquivo.txt>

ou... (pipe com a entrada .txt):

$ cat <arquivo.txt> | ./ep1
____________________________________________________________________________

TESTES:


Teste 1 (Python - ola mundo):

$ python3 ep1.py
Ola mundo!
7ea2319be0d038908161b4e8c26bfc7a

Teste 1 (Assembly - ola mundo):

$ ./ep1
Ola mundo!
7ea2319be0d038908161b4e8c26bfc7a
____________________________________________________________________________

Teste 2 (Python - texto100000):

$ python3 ep1.py < texto100000.txt
67bd95a004dea32fcb8e7bcb97c723fe

Teste 2 (Assembly - texto100000):

$ ./ep1 < texto100000.txt
67bd95a004dea32fcb8e7bcb97c723fe

Teste 2 (Assembly - texto100000):  *

$ cat texto100000.txt | ./ep1
79ac9fbb57ebb609764e0ac1c1b1fe9d

* Obs.: para textos com mais de 65536 caracteres (65536 bytes) o uso do 
comando  cat produz resultados diferentes, pois a fila do pipe "|" 
comporta no maximo essa quantidade de bytes, conforme mencionado
pelo monitor da disciplina, Lucas Seiki Oshiro, no forum da mesma.

____________________________________________________________________________

Teste 3 (Python - que tistreza):

$ python3 ep1.py < entrada3-enunciado.txt 
878561c1134d1fd53e9b36822e1914cc

Teste 3 (Assembly - que tistreza):

$ cat entrada3-enunciado.txt | ./ep1
878561c1134d1fd53e9b36822e1914cc
____________________________________________________________________________

Teste 4 (Python - harry potter):

$ python3 ep1.py < entrada6-enunciado.txt 
896a06f28a29a4352544dc231913740a

Teste 4 (Assembly - harry potter):

$ cat entrada6-enunciado.txt | ./ep1
896a06f28a29a4352544dc231913740a
____________________________________________________________________________

DEPENDˆENCIAS:

Para compilar e rodar os programas, voce deve ter instalado em sua maquina
o NASM (Netwide Assembler) para montar o programa em Assembly, o GNU ld
para linkar o arquivo .o, e o interpretador Python para rodar os arquivos
em Python. Segue informacoes destes, da arquitetura do processador, do
processador, e do sistema operacional respectivos em que os programas 
foram implementados:


NASM version 2.16.01 compiled on Aug 15 2023
GNU ld (GNU Binutils for Ubuntu) 2.38
Python 3.10.6

x86-64
AMD Dual-Core A4-3300M APU 
Linux Mint 21.1
