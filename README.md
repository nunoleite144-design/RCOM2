# RCOM Lab 2 — FTP Download Client

Projeto desenvolvido no âmbito da unidade curricular de **Redes de Computadores**.

O objetivo principal é implementar, em linguagem C, uma aplicação chamada `download` capaz de descarregar ficheiros a partir de servidores FTP, usando diretamente sockets TCP e comandos do protocolo FTP.

---

## Índice

- [Objetivo do projeto](#objetivo-do-projeto)
- [Funcionalidades implementadas](#funcionalidades-implementadas)
- [Estrutura do repositório](#estrutura-do-repositório)
- [Compilação](#compilação)
- [Execução](#execução)
- [Exemplos de utilização](#exemplos-de-utilização)
- [Funcionamento da aplicação](#funcionamento-da-aplicação)
- [Sequência FTP usada](#sequência-ftp-usada)
- [Notas para o laboratório](#notas-para-o-laboratório)
- [Troubleshooting](#troubleshooting)

---

## Objetivo do projeto

A primeira parte do Lab 2 consiste no desenvolvimento de uma aplicação de download baseada no protocolo FTP.

A aplicação deve receber um URL FTP no seguinte formato:

```bash
./download ftp://[user:password@]host/path/to/file
```

A partir desse URL, o programa deve:

1. interpretar os vários campos do URL;
2. resolver o endereço IP do servidor através de DNS;
3. abrir uma ligação TCP de controlo para o servidor FTP;
4. autenticar o utilizador;
5. entrar em modo passivo;
6. abrir a ligação TCP de dados;
7. pedir o ficheiro ao servidor;
8. guardar o ficheiro no diretório atual.

---

## Funcionalidades implementadas

- Parse de URLs FTP com e sem credenciais explícitas.
- Suporte para autenticação `anonymous` por omissão.
- Resolução DNS com `gethostbyname`.
- Criação de sockets TCP com a API Berkeley Sockets.
- Ligação de controlo FTP na porta `21`.
- Envio de comandos FTP terminados com `CRLF`.
- Leitura de respostas FTP, incluindo respostas multi-linha.
- Modo binário com `TYPE I`.
- Modo passivo com `PASV`.
- Cálculo automático do IP e porto da ligação de dados.
- Transferência do ficheiro com `RETR`.
- Escrita do ficheiro recebido no diretório atual.
- Tratamento básico de erros em cada fase.

---

## Estrutura do repositório

```text
RCOM2/
├── src/
│   └── download.c          # Código principal da aplicação FTP
├── examples/
│   ├── clientTCP.c         # Exemplo base de cliente TCP
│   └── getip.c             # Exemplo base de resolução DNS
├── captures/
│   └── .gitkeep            # Pasta para capturas Wireshark selecionadas
├── docs/                   # Documentação auxiliar
├── report/
│   └── relatorio.md        # Estrutura inicial do relatório
├── Makefile                # Regras de compilação
├── .gitignore              # Ficheiros ignorados pelo Git
└── README.md
```

---

## Compilação

Para compilar o projeto, basta executar:

```bash
make
```

Isto gera o executável:

```bash
./download
```

Também é possível compilar manualmente com:

```bash
gcc -Wall -Wextra -pedantic -std=c11 src/download.c -o download
```

Para limpar o executável gerado:

```bash
make clean
```

---

## Execução

Formato geral:

```bash
./download ftp://[user:password@]host/path/to/file
```

Quando o URL não inclui utilizador e palavra-passe, a aplicação usa automaticamente:

```text
user: anonymous
password: anonymous@
```

---

## Exemplos de utilização

Exemplo com login anonymous explícito:

```bash
./download ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html
```

Exemplo com login anonymous implícito:

```bash
./download ftp://mirrors.up.pt/debian/README.html
```

Exemplo esperado no laboratório, adaptando o caminho indicado no enunciado:

```bash
./download ftp://ftp.netlab.fe.up.pt/pub/...
```

---

## Funcionamento da aplicação

A aplicação segue a arquitetura típica de um cliente FTP simples.

```text
          Ligação TCP de controlo, porta 21
Cliente  ------------------------------------>  Servidor FTP
          USER, PASS, TYPE I, PASV, RETR, QUIT

          Ligação TCP de dados, porto PASV
Cliente  <------------------------------------  Servidor FTP
          Conteúdo do ficheiro
```

O FTP usa duas ligações TCP diferentes:

1. **Ligação de controlo** — usada para comandos e respostas FTP.
2. **Ligação de dados** — usada para transferir o conteúdo do ficheiro.

No modo passivo, o servidor indica ao cliente qual o IP e porto onde deve abrir a ligação de dados.

---

## Sequência FTP usada

A sequência principal implementada é:

```text
Servidor -> 220 Service ready
Cliente  -> USER <user>
Servidor -> 331 Password required ou 230 Login successful
Cliente  -> PASS <password>
Servidor -> 230 Login successful
Cliente  -> TYPE I
Servidor -> 200 Switching to Binary mode
Cliente  -> PASV
Servidor -> 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
Cliente  -> abre ligação TCP para h1.h2.h3.h4 : p1*256+p2
Cliente  -> RETR <path>
Servidor -> 150 Opening data connection
Servidor -> envia ficheiro pela ligação de dados
Servidor -> 226 Transfer complete
Cliente  -> QUIT
Servidor -> 221 Goodbye
```

O porto da ligação de dados é calculado por:

```text
port = p1 * 256 + p2
```

---

## Notas para o laboratório

Durante a demonstração e análise com Wireshark, devem ser observadas pelo menos duas ligações TCP:

- a ligação de controlo FTP, normalmente para a porta `21`;
- a ligação de dados FTP, para o porto indicado na resposta ao comando `PASV`.

No Wireshark, é útil analisar:

- o estabelecimento da ligação TCP, com `SYN`, `SYN-ACK` e `ACK`;
- os comandos FTP enviados na ligação de controlo;
- a resposta ao comando `PASV`;
- a transferência do ficheiro na ligação de dados;
- o encerramento das ligações TCP;
- eventuais retransmissões ou alterações no throughput.

---

## Troubleshooting

### `make: command not found`

O ambiente não tem `make` instalado. Compilar manualmente:

```bash
gcc -Wall -Wextra -pedantic -std=c11 src/download.c -o download
```

### `gethostbyname: Unknown host`

O nome do servidor não foi resolvido. Confirmar:

- se o URL está correto;
- se há ligação à rede;
- se o DNS está configurado corretamente.

### `connect: Connection refused`

O servidor recusou a ligação. Possíveis causas:

- servidor FTP indisponível;
- porto incorreto;
- firewall ou rede do laboratório a bloquear o acesso.

### Erro após `PASV`

Confirmar se a resposta do servidor tem o formato:

```text
227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
```

A aplicação depende desses valores para abrir a ligação de dados.

### Ficheiro descarregado com tamanho 0

Possíveis causas:

- caminho do ficheiro incorreto;
- permissões insuficientes no servidor;
- erro antes da resposta final `226 Transfer complete`.

---

## Avisos importantes

- Não colocar passwords reais no código nem no README.
- Não subir ficheiros `.pcap` ou `.pcapng` grandes sem necessidade.
- Manter o repositório privado até à entrega para evitar problemas de cópia ou plágio.
- Confirmar sempre no laboratório se o URL usado é o indicado pelo docente.

---

## Autor

Projeto desenvolvido para o Lab 2 de Redes de Computadores.
