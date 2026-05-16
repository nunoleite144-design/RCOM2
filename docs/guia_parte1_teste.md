# Guia prático — Teste da Parte 1: aplicação `download`

Este guia é para usares na FEUP para **testar a Parte 1** do projeto: a aplicação `download` em C que descarrega um ficheiro por FTP.

A ideia é terminares o teste com:

1. o programa compilado;
2. um download concluído com sucesso;
3. o ficheiro descarregado guardado no PC;
4. uma captura Wireshark `.pcapng` com os pacotes FTP/TCP;
5. prints suficientes para depois escrever a secção da Parte 1 no relatório.

---

# 1. Informação importante da bancada

Na folha da bancada aparece explicitamente:

```text
FTP: 172.16.1.10
```

Portanto, **o servidor FTP a usar no laboratório é**:

```text
172.16.1.10
```

O URL final para testar o teu programa vai ter esta forma:

```bash
./download ftp://172.16.1.10/caminho/do/ficheiro
```

O único detalhe que tens de descobrir no momento é o **caminho exato do ficheiro** existente no servidor FTP.

---

# 2. Objetivo da Parte 1

A aplicação deve receber um URL FTP neste formato:

```bash
./download ftp://[user:password@]host/path/to/file
```

E realizar automaticamente:

```text
1. Parse do URL
2. Resolução/identificação do servidor
3. Ligação TCP de controlo à porta 21
4. Login com USER/PASS
5. TYPE I
6. PASV
7. Ligação TCP de dados
8. RETR do ficheiro
9. Escrita do ficheiro no disco
10. QUIT
```

---

# 3. Onde fazer o teste

Faz o teste no **tux93**, que é o tux que tens usado como máquina principal.

A Parte 1 deve ser testada **depois de a rede estar funcional o suficiente para o tux93 alcançar o FTP `172.16.1.10`**.

Antes de correr o teu programa, confirma a conectividade:

```bash
ping -c 4 172.16.1.10
```

Se o ping responder, ótimo. Se não responder, não concluas logo que o FTP não funciona; o servidor pode bloquear ICMP. Nesse caso, continua para o teste manual com `ftp`.

---

# 4. Levar/obter o código no tux93

## Opção A — Clonar o GitHub

No terminal:

```bash
cd ~
git clone https://github.com/nunoleite144-design/RCOM2.git
cd RCOM2
```

Se a pasta já existir:

```bash
cd ~/RCOM2
```

Se o GitHub pedir autenticação e te atrapalhar, usa a opção B.

## Opção B — Levar numa pen ou transferir manualmente

Leva pelo menos:

```text
src/download.c
Makefile
```

Ou copia a pasta inteira `RCOM2`.

---

# 5. Compilar a aplicação

Dentro da pasta do projeto:

```bash
make
```

Deve gerar o executável:

```text
download
```

Confirmar:

```bash
ls -lh download
```

## Se `make` falhar

Compila manualmente:

```bash
gcc -Wall -Wextra -pedantic -std=c11 src/download.c -o download
```

---

# 6. Descobrir um ficheiro real no servidor FTP

Como o servidor é conhecido (`172.16.1.10`) mas o ficheiro exato pode variar, entra manualmente no FTP para ver o conteúdo.

No `tux93`:

```bash
ftp 172.16.1.10
```

Quando o servidor pedir credenciais, usa as que forem aceites pelo servidor. Se estiver configurado para acesso anónimo, tenta:

```text
Name: anonymous
Password: anonymous@
```

Depois, dentro do cliente FTP:

```text
ftp> pwd
ftp> ls
```

Se vires uma pasta relevante, entra nela:

```text
ftp> cd nome_da_pasta
ftp> pwd
ftp> ls
```

O objetivo é encontrares **um ficheiro concreto**, não uma pasta.

Exemplo genérico:

```text
ftp> pwd
/pub
ftp> ls
README.txt
```

Nesse exemplo, o URL para o teu programa seria:

```bash
./download ftp://172.16.1.10/pub/README.txt
```

Quando terminares a exploração manual:

```text
ftp> quit
```

---

# 7. Primeiro teste rápido do teu programa

Agora testa o teu programa com o caminho que descobriste.

Exemplo genérico:

```bash
./download ftp://172.16.1.10/pub/README.txt
```

Substitui `/pub/README.txt` pelo caminho real que encontrares.

---

# 8. O que deves ver no terminal se correr bem

O terminal deve mostrar algo semelhante a:

```text
Host: 172.16.1.10
Path: /...
Output file: ...
Server IP: 172.16.1.10
< 220 ...
> USER ...
< 331 ...
> PASS ...
< 230 ...
> TYPE I
< 200 ...
> PASV
< 227 Entering Passive Mode (...)
Passive data address: IP:PORT
> RETR ...
< 150 ...
Saved N bytes to ficheiro
< 226 Transfer complete.
> QUIT
< 221 Goodbye.
```

O mais importante é aparecer:

```text
Saved ... bytes to ...
226 Transfer complete
```

---

# 9. Confirmar que o ficheiro foi mesmo descarregado

Depois do download:

```bash
ls -lh
```

Confirma que apareceu o ficheiro descarregado.

Se o ficheiro for de texto, podes fazer:

```bash
head nome_do_ficheiro
```

---

# 10. Descobrir em que interface capturar no Wireshark

Antes de fazer a captura final, descobre por onde o `tux93` vai comunicar com o FTP.

Como o servidor é:

```text
172.16.1.10
```

faz:

```bash
ip route get 172.16.1.10
```

O resultado indica a interface de saída, por exemplo:

```text
dev if_e1
```

ou outra interface.

**É essa interface que deves selecionar no Wireshark.**

---

# 11. Captura Wireshark do teste final

Agora faz o teste que interessa para o relatório.

## Passo 1 — Abrir Wireshark

```bash
sudo wireshark &
```

## Passo 2 — Selecionar a interface certa

Seleciona a interface indicada por:

```bash
ip route get 172.16.1.10
```

## Passo 3 — Começar a captura

Clica em **Start Capture**.

## Passo 4 — Executar de novo o download

Corre novamente o teu programa com o URL real que já validaste.

Exemplo genérico:

```bash
./download ftp://172.16.1.10/pub/README.txt
```

---

# 12. Parar e guardar a captura

Depois de o download terminar:

1. parar a captura no Wireshark;
2. `File -> Save As`;
3. guardar com nome claro.

Nome recomendado:

```text
parte1_download_ftp_172.16.1.10.pcapng
```

---

# 13. Filtros úteis no Wireshark

Depois da captura, podes usar:

```text
ftp || tcp
```

ou:

```text
tcp.port == 21
```

ou ainda:

```text
ip.addr == 172.16.1.10
```

---

# 14. O que tens de observar no Wireshark

A captura deve permitir mostrar:

## A. Ligação TCP de controlo

Para a porta FTP:

```text
TCP destino porta 21
```

Deves encontrar:

```text
SYN
SYN-ACK
ACK
```

## B. Comandos FTP na ligação de controlo

Deves ver, na ordem esperada:

```text
USER
PASS
TYPE I
PASV
RETR
QUIT
```

## C. Resposta ao PASV

Procura a linha:

```text
227 Entering Passive Mode (...)
```

A resposta PASV indica o IP/porto para a ligação de dados.

## D. Ligação TCP de dados

Depois do `PASV`, deve surgir uma **segunda ligação TCP** para uma porta diferente da 21.

É por essa ligação que o ficheiro é transferido.

## E. Final da transferência

Na ligação de controlo deve aparecer:

```text
226 Transfer complete
```

---

# 15. Prints que vale a pena tirar

Não precisas de tirar 20 prints. Tira estes 4 ou 5 bem escolhidos:

## Print 1 — Terminal com download concluído

De preferência com:

```text
PASV
RETR
Saved ... bytes
226 Transfer complete
```

## Print 2 — Wireshark com comandos FTP

Mostrar na lista de pacotes algo com:

```text
USER
PASS
PASV
RETR
```

## Print 3 — Resposta PASV

Seleciona o pacote onde aparece:

```text
227 Entering Passive Mode (...)
```

## Print 4 — TCP handshake da ligação de controlo

Mostrar:

```text
SYN
SYN-ACK
ACK
```

## Print 5 — Segunda ligação TCP de dados

Mostrar que existe uma conexão TCP diferente da porta 21 usada para transferir dados.

---

# 16. O que guardar para o relatório

No fim da Parte 1, deves ter guardado:

```text
1. O URL usado no teste
2. A captura .pcapng
3. 4 ou 5 prints bons
4. O ficheiro descarregado
5. O output do terminal
```

Nome recomendado para organizar:

```text
EXPERIÊNCIAS/PARTE1/
├── parte1_download_ftp_172.16.1.10.pcapng
├── screenshot_terminal_download.png
├── screenshot_ftp_commands.png
├── screenshot_pasv.png
├── screenshot_tcp_handshake.png
└── ficheiro_descarregado
```

---

# 17. Troubleshooting rápido

## `connect: Connection refused`

O servidor FTP recusou a ligação ou o serviço não está disponível naquele momento.

Testa primeiro:

```bash
ftp 172.16.1.10
```

## O programa fica preso depois de `PASV`

Pode haver problema na ligação de dados.

Confirma se o servidor responde com:

```text
227 Entering Passive Mode (...)
```

## `RETR` dá erro

O caminho do ficheiro pode estar errado.

Volta ao cliente FTP manual e confirma:

```text
ftp> pwd
ftp> ls
```

## Não sabes que ficheiro escolher

Escolhe um ficheiro pequeno e simples, idealmente `.txt`, `.html` ou semelhante, para o teste ser rápido e fácil de validar.

---

# 18. Checklist rápida para amanhã

```text
[ ] Entrei no servidor FTP 172.16.1.10 manualmente
[ ] Descobri um ficheiro real e anotei o path completo
[ ] Levei/cloniei o repo RCOM2
[ ] Compilei com make
[ ] Fiz um teste rápido do ./download
[ ] Confirmei por que interface sai o tráfego para 172.16.1.10
[ ] Abri Wireshark na interface certa
[ ] Corri o download oficial
[ ] O ficheiro apareceu no disco
[ ] Guardei .pcapng
[ ] Tirei prints do terminal e do Wireshark
[ ] Guardei tudo com nomes claros
```

---

# 19. Mini-resumo mental

```text
Parte 1 = provar que a app funciona contra o FTP 172.16.1.10.

Preciso de:
- descobrir um ficheiro no FTP;
- compilar;
- descarregar com ./download;
- guardar captura Wireshark;
- mostrar FTP controlo + dados;
- guardar evidências para o relatório.
```
