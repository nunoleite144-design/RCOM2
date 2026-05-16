# Guia prático — Teste da Parte 1: aplicação `download`

Este guia é para usares amanhã na FEUP para **testar a Parte 1** do projeto: a aplicação `download` em C que descarrega um ficheiro por FTP.

A ideia é terminares o teste com:

1. o programa compilado;
2. um download concluído com sucesso;
3. o ficheiro descarregado guardado no PC;
4. uma captura Wireshark `.pcapng` com os pacotes FTP/TCP;
5. prints suficientes para depois escrever a secção da Parte 1 no relatório.

---

# 1. Objetivo da Parte 1

A aplicação deve receber um URL FTP neste formato:

```bash
./download ftp://[user:password@]host/path/to/file
```

E realizar automaticamente:

```text
1. Parse do URL
2. Resolução DNS do host
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

# 2. Onde fazer o teste

Faz o teste num tux da FEUP, de preferência no `tux93`, porque é o tux que já tens usado como máquina principal do projeto.

## Nota importante sobre a interface de rede

Se amanhã **só fores repetir a Experiência 3** e ainda não tiveres montado a topologia completa com RC/NAT, o tráfego FTP pode sair pela **interface de gestão da FEUP**, normalmente algo como:

```text
if_mng
```

ou outra interface com IP `10.227.20.X`.

Por isso, **não assumas automaticamente que a captura deve ser na `if_e1`**. Antes de capturar, confirma por onde o PC chega ao servidor.

---

# 3. Levar/obter o código no tux

## Opção A — Clonar o GitHub

No terminal:

```bash
git clone https://github.com/nunoleite144-design/RCOM2.git
cd RCOM2
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

# 4. Compilar a aplicação

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

# 5. Primeiro teste rápido sem Wireshark

Antes de começares a captura, faz um teste simples só para ver se o programa funciona.

## Caso o professor te dê um URL concreto do servidor netlab

Usa exatamente esse URL:

```bash
./download ftp://ftp.netlab.fe.up.pt/pub/...
```

## Caso ainda não saibas o ficheiro exato

Podes fazer um teste de sanidade com o ficheiro que aparece no guião da primeira aula:

```bash
./download ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html
```

Ou, se o teu programa usar anonymous por omissão:

```bash
./download ftp://mirrors.up.pt/debian/README.html
```

---

# 6. O que deves ver no terminal se correr bem

O terminal deve mostrar algo semelhante a:

```text
Host: ...
Path: ...
Output file: ...
Server IP: ...
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

# 7. Confirmar que o ficheiro foi mesmo descarregado

Depois do download:

```bash
ls -lh
```

Confirma que apareceu o ficheiro descarregado.

Se o ficheiro for de texto, podes fazer:

```bash
head ficheiro
```

ou:

```bash
cat ficheiro | head
```

---

# 8. Descobrir em que interface capturar no Wireshark

Antes de fazer a captura final, descobre por onde o tux vai comunicar com o servidor.

## Ver a rota para o servidor

Se fores usar `ftp.netlab.fe.up.pt`:

```bash
getent hosts ftp.netlab.fe.up.pt
```

Aponta o IP que aparecer.

Depois:

```bash
ip route get IP_DO_SERVIDOR
```

Exemplo:

```bash
ip route get 172.16.1.10
```

O resultado costuma indicar a interface de saída, por exemplo:

```text
dev if_mng
```

ou:

```text
dev if_e1
```

**É essa interface que deves selecionar no Wireshark.**

---

# 9. Captura Wireshark do teste final

Agora faz o teste que interessa para o relatório.

## Passo 1 — Abrir Wireshark

```bash
sudo wireshark &
```

## Passo 2 — Selecionar a interface certa

Seleciona a interface que descobriste com:

```bash
ip route get IP_DO_SERVIDOR
```

## Passo 3 — Começar a captura

Clica em **Start Capture**.

## Passo 4 — Executar de novo o download

Usa o URL que queres guardar como teste oficial.

Idealmente, usa o servidor pedido pelo professor:

```bash
./download ftp://ftp.netlab.fe.up.pt/pub/...
```

Se ainda não tiveres o ficheiro exato, usa temporariamente o ficheiro de teste:

```bash
./download ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html
```

---

# 10. Parar e guardar a captura

Depois de o download terminar:

1. parar a captura no Wireshark;
2. `File -> Save As`;
3. guardar com nome claro.

Nome recomendado:

```text
parte1_download_ftp.pcapng
```

ou, se for netlab:

```text
parte1_download_netlab.pcapng
```

---

# 11. Filtros úteis no Wireshark

Depois da captura, podes usar:

```text
ftp || tcp
```

ou:

```text
tcp.port == 21
```

Se já souberes o IP do servidor:

```text
ip.addr == IP_DO_SERVIDOR
```

---

# 12. O que tens de observar no Wireshark

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

# 13. Prints que vale a pena tirar

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

# 14. O que guardar para o relatório

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
├── parte1_download_ftp.pcapng
├── screenshot_terminal_download.png
├── screenshot_ftp_commands.png
├── screenshot_pasv.png
├── screenshot_tcp_handshake.png
└── ficheiro_descarregado
```

---

# 15. Troubleshooting rápido

## `gethostbyname: Unknown host`

O DNS falhou. Testa:

```bash
getent hosts ftp.netlab.fe.up.pt
```

Se não resolver, pergunta ao professor se o DNS/rede está disponível naquela máquina.

## `connect: Connection refused`

O servidor FTP recusou a ligação ou o servidor não está acessível nesse momento.

Confirma o URL e volta a tentar.

## O programa fica preso depois de `PASV`

Pode haver problema na ligação de dados.

Confirma se o servidor responde com:

```text
227 Entering Passive Mode (...)
```

## `RETR` dá erro

O caminho do ficheiro pode estar errado.

Confirma o caminho exato pedido pelo professor.

---

# 16. Checklist rápida para amanhã

```text
[ ] Levei/cloniei o repo RCOM2
[ ] Compilei com make
[ ] Fiz um teste rápido de funcionamento
[ ] Confirmei por que interface sai o tráfego
[ ] Abri Wireshark na interface certa
[ ] Corri o download oficial
[ ] O ficheiro apareceu no disco
[ ] Guardei .pcapng
[ ] Tirei prints do terminal e do Wireshark
[ ] Guardei tudo com nomes claros
```

---

# 17. Mini-resumo mental

```text
Parte 1 = provar que a app funciona.

Preciso de:
- compilar;
- descarregar ficheiro;
- guardar captura Wireshark;
- mostrar FTP controlo + dados;
- guardar evidências para o relatório.
```
