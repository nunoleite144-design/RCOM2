# RCOM Lab 2 — FTP Download Client

Projeto de Redes de Computadores para o Lab 2.

## Objetivo

Implementar em C uma aplicação `download` que descarrega um ficheiro através de FTP, usando sockets TCP e modo passivo.

Formato esperado:

```bash
./download ftp://[user:password@]host/path/to/file
```

Exemplos:

```bash
./download ftp://ftp.netlab.fe.up.pt/pub/ficheiro.txt
./download ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html
```

## Estrutura

```text
rcom-lab2/
├── src/
│   └── download.c
├── examples/
│   ├── clientTCP.c
│   └── getip.c
├── captures/
│   └── .gitkeep
├── docs/
├── report/
│   └── relatorio.md
├── Makefile
├── .gitignore
└── README.md
```

## Compilar

```bash
make
```

ou diretamente:

```bash
gcc -Wall -Wextra -pedantic -std=c11 src/download.c -o download
```

## Executar

```bash
./download ftp://anonymous:anonymous@mirrors.up.pt/debian/README.html
```

No laboratório, usar o URL indicado no enunciado, por exemplo:

```bash
./download ftp://ftp.netlab.fe.up.pt/pub/...
```

## Sequência FTP implementada

A aplicação faz, de forma automática:

1. Parse do URL FTP;
2. Resolução DNS do hostname com `gethostbyname`;
3. Abertura da ligação TCP de controlo para a porta 21;
4. Login com `USER` e `PASS`;
5. Seleção de modo binário com `TYPE I`;
6. Entrada em modo passivo com `PASV`;
7. Abertura da ligação TCP de dados;
8. Pedido do ficheiro com `RETR`;
9. Escrita do ficheiro no diretório atual;
10. Fecho da sessão com `QUIT`.

## Subir para GitHub

Criar primeiro um repositório vazio no GitHub, de preferência privado.

Depois, dentro desta pasta:

```bash
git init
git add .
git commit -m "Initial FTP download client"
git branch -M main
git remote add origin https://github.com/TEU-USER/rcom-lab2.git
git push -u origin main
```

Substituir `TEU-USER` pelo teu username do GitHub.

## Notas

- Não colocar passwords reais no código nem no README.
- Não subir ficheiros `.pcap`/`.pcapng` grandes sem necessidade.
- Se o repositório for público, evitar colocar PDFs ou materiais da cadeira sem autorização.
