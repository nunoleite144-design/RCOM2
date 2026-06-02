# RCOM — Lab 2 / Parte 2 · Guião completo (do zero) — Exp 4, 5 e 6

**Grupo 9 (Y = 9)** · tux**93**, tux**94**, tux**92** · Router comercial: **RC** (MikroTik/RouterOS) · Switch: MikroTik

> Este guião parte do zero — sem nada configurado. Segue a ordem: setup físico → switch → tuxes → Exp 4 → Exp 5 → Exp 6.

---

## 0. Mapa de endereçamento (Y = 9)

| Rede | Bridge | Equipamento · interface | IP / máscara |
|---|---|---|---|
| `172.16.90.0/24` | **bridge90** | tux93 · eth1 | `172.16.90.1/24` |
| | | tux94 · eth1 | `172.16.90.254/24` |
| `172.16.91.0/24` | **bridge91** | tux94 · eth2 | `172.16.91.253/24` |
| | | tux92 · eth1 | `172.16.91.1/24` |
| | | RC · ether2 (interno/NAT) | `172.16.91.254/24` |
| `172.16.1.0/24` | (rede lab) | RC · ether1 (externo) | `172.16.1.99/24` |
| | | FTP `ftp.netlab.fe.up.pt` | `172.16.1.10` |
| | | Gateway lab → Internet | `172.16.1.254` |
| `10.227.20.0/24` | (gestão) | DNS `services.netlab.fe.up.pt` | `10.227.20.3` |

## 0.1 Portos do switch (MikroTik) — Grupo 9

| Porto switch | Equipamento ligado | Bridge |
|---|---|---|
| ether7 | tux93 · eth1 | bridge90 |
| ether10 | tux94 · eth1 | bridge90 |
| ether6 | tux92 · eth1 | bridge91 |
| ether11 | tux94 · eth2 | bridge91 |
| ether13 | RC · ether2 | bridge91 |

> **Confirma na sala:** o IP externo do RC (`172.16.1.99`) e o porto de patch (`P9.12`) podem variar conforme anunciado. Substitui se necessário.

---

## 1. Ligações físicas (cabos)

**Antes de ligar qualquer cabo, confirma quais os portos do patch panel do teu grupo.**

1. tux93 eth1 → porto **ether7** do switch
2. tux94 eth1 → porto **ether10** do switch
3. tux94 eth2 → porto **ether11** do switch
4. tux92 eth1 → porto **ether6** do switch
5. RC ether2 → porto **ether13** do switch
6. RC ether1 → rede do lab no patch **P9.12** (externo, com NAT)

---

## 2. Reset e configuração do switch (MikroTik)

Acede ao switch via consola série: cabo série de um dos tux para o switch, abre **GTKterm** (`/dev/ttyS0`, 115200 bps, 8N1). Login: `admin`, password: vazia (Enter).

### 2.1 Reset (limpa tudo)

```routeros
/system reset-configuration
```
Responde `y` e Enter. O switch reinicia — espera ~30 s e volta a ligar.

### 2.2 Criar bridges

```routeros
/interface bridge add name=bridge90
/interface bridge add name=bridge91
/interface bridge print          # confirmar as duas bridges
```

### 2.3 Remover portos da bridge padrão e adicionar às bridges corretas

```routeros
# Remover dos portos da bridge default (pode já não existir após reset, mas faz na mesma)
/interface bridge port remove [find interface=ether7]
/interface bridge port remove [find interface=ether10]
/interface bridge port remove [find interface=ether6]
/interface bridge port remove [find interface=ether11]
/interface bridge port remove [find interface=ether13]

# Adicionar à bridge90
/interface bridge port add bridge=bridge90 interface=ether7
/interface bridge port add bridge=bridge90 interface=ether10

# Adicionar à bridge91
/interface bridge port add bridge=bridge91 interface=ether6
/interface bridge port add bridge=bridge91 interface=ether11
/interface bridge port add bridge=bridge91 interface=ether13

# Verificar
/interface bridge port print brief
```

Deves ver ether7 e ether10 em bridge90; ether6, ether11 e ether13 em bridge91.

---

## 3. Configuração dos tuxes (do zero)

Em cada tux abre um terminal. Podes usar `ssh` a partir de um tux para os outros, ou trabalhar em terminais separados.

### 3.1 tux93

```bash
# Configurar interface
ifconfig eth1 172.16.90.1/24

# Verificar
ifconfig eth1
route -n    # deve ter 172.16.90.0/24 directo
```

### 3.2 tux94 — router Linux (configuração mais extensa)

```bash
# Configurar as duas interfaces
ifconfig eth1 172.16.90.254/24
ifconfig eth2 172.16.91.253/24

# Activar forwarding IP (tornar o tux94 num router)
sysctl net.ipv4.ip_forward=1

# Desactivar ignorar broadcast ICMP (para pings broadcast funcionarem)
sysctl net.ipv4.icmp_echo_ignore_broadcasts=0

# Verificar
ifconfig eth1
ifconfig eth2
sysctl net.ipv4.ip_forward    # deve mostrar 1
route -n    # deve ter 172.16.90.0/24 e 172.16.91.0/24 directos
```

### 3.3 tux92

```bash
# Configurar interface
ifconfig eth1 172.16.91.1/24

# Verificar
ifconfig eth1
route -n    # deve ter 172.16.91.0/24 directo
```

### 3.4 Rotas entre sub-redes (Exp 3 — necessário antes da Exp 4)

```bash
# tux93: alcançar a sub-rede 172.16.91.0/24 via tux94
route add -net 172.16.91.0/24 gw 172.16.90.254

# tux92: alcançar a sub-rede 172.16.90.0/24 via tux94
route add -net 172.16.90.0/24 gw 172.16.91.253
```

### 3.5 Verificação rápida antes da Exp 4

```bash
# De tux93, pinga tux94 e tux92
ping -c 3 172.16.90.254    # tux94 eth1 — OK directo
ping -c 3 172.16.91.253    # tux94 eth2 — OK via tux94
ping -c 3 172.16.91.1      # tux92      — OK via tux94
```

Se tudo responder, o setup base está completo.

---

## EXPERIÊNCIA 4 — Router comercial (RC) + NAT

**Objectivo:** introduzir o RC na bridge91, configurar NAT, estudar rotas estáticas e ICMP redirect.

### 4.1 Confirmar ether13 na bridge91

Já foi feito na secção 2.3. Confirma:

```routeros
/interface bridge port print brief    # ether13 deve estar em bridge91
```

### 4.2 Configurar o RC (consola série)

Acede ao RC via consola série (mesmo método do switch — abre GTKterm em tux92 ou tux93, `/dev/ttyS0`, 115200). Login: `admin`, password: conforme anunciado (tipicamente vazia).

```routeros
# (Opcional — reset limpo se o router vier sujo)
# /system reset-configuration no-defaults=yes skip-backup=yes

# Endereços IP das interfaces
/ip address add address=172.16.91.254/24 interface=ether2
/ip address add address=172.16.1.99/24   interface=ether1

# Rota para rede interna 172.16.90.0/24 (via tux94.eth2)
/ip route add dst-address=172.16.90.0/24 gateway=172.16.91.253

# Rota default (Internet via gateway do lab)
/ip route add dst-address=0.0.0.0/0 gateway=172.16.1.254

# Confirmar
/ip address print
/ip route print
```

**NAT (masquerade)** — confirmar se já existe (normalmente vem por defeito):

```routeros
/ip firewall nat print
# Se a regra masquerade não existir, criar:
/ip firewall nat add chain=srcnat action=masquerade out-interface=ether1
```

### 4.3 Rotas adicionais nos tuxes (passo 2 do guião)

```bash
# tux93: alcançar 172.16.1.0/24 (rede do FTP) via tux94
route add -net 172.16.1.0/24 gw 172.16.90.254

# tux94: alcançar 172.16.1.0/24 via RC
route add -net 172.16.1.0/24 gw 172.16.91.254

# tux92: alcançar 172.16.1.0/24 via RC
route add -net 172.16.1.0/24 gw 172.16.91.254
# (a rota para 172.16.90.0/24 já existe do passo 3.4)
```

### 4.4 Verificar conectividade (passo 3)

Inicia **Wireshark** em `tux93.eth1`. No **tux93**:

```bash
ping -c 3 172.16.90.254    # tux94 eth1
ping -c 3 172.16.91.253    # tux94 eth2
ping -c 3 172.16.91.1      # tux92
ping -c 3 172.16.91.254    # RC ether2
ping -c 3 172.16.1.10      # FTP server (testa NAT)
```

Todos devem responder. O ping ao FTP (172.16.1.10) só funciona com NAT activo.

### 4.5 ICMP Redirect (passo 4)

No **tux92**, Wireshark em `tux92.eth1`:

```bash
# 1. Desactivar aceitação de redirects
sysctl net.ipv4.conf.eth1.accept_redirects=0
sysctl net.ipv4.conf.all.accept_redirects=0

# 2. Forçar tux92 a usar RC como gateway para 172.16.90.0/24 (rota subótima)
route del -net 172.16.90.0/24
route add -net 172.16.90.0/24 gw 172.16.91.254    # via RC

# 3. Observar o caminho
ping -c 5 172.16.90.1       # tux93
traceroute 172.16.90.1      # deve mostrar 2 saltos: RC e tux94
```

**O que observar:** tux92 → RC → tux94 → tux93 (caminho em "U" na mesma sub-rede 91). O RC emite um **ICMP Redirect** a dizer "usa 172.16.91.253 directamente", mas como `accept_redirects=0`, o tux92 **ignora** — o caminho mantém-se subótimo. O `traceroute` mostra **2 saltos**.

```bash
# 4. Repor rota óptima e comparar
route del -net 172.16.90.0/24
route add -net 172.16.90.0/24 gw 172.16.91.253    # via tux94 (directo)
traceroute 172.16.90.1      # agora 1 salto
```

```bash
# 5. Reactivar redirects e repetir com rota subótima
sysctl net.ipv4.conf.eth1.accept_redirects=1
sysctl net.ipv4.conf.all.accept_redirects=1
route del -net 172.16.90.0/24
route add -net 172.16.90.0/24 gw 172.16.91.254    # via RC outra vez

ping -c 5 172.16.90.1
route -n    # aparece rota de host 172.16.90.1 via 172.16.91.253 (injectada pelo redirect)
```

**Com redirects ligados:** após receber o ICMP Redirect, o tux92 instala uma rota de host e passa a enviar directamente para tux94 — o RC deixa de intervir.

### 4.6 NAT — com e sem (passos 5–7)

**Com NAT (estado actual)** — em tux93:

```bash
ping -c 5 172.16.1.10       # FTP server — funciona (masquerade activo)
```

Caminho: tux93 → tux94 → RC → (RC substitui src por `172.16.1.99`) → FTP.

**Desligar NAT** no RC:

```routeros
/ip firewall nat disable 0    # (ou o número da regra no print)
```

```bash
# tux93
ping -c 5 172.16.1.10       # FALHA — sem resposta
```

**Porquê falha:** sem NAT, o FTP recebe um pacote com origem `172.16.90.1` (IP privado) e não tem rota de retorno para esse endereço — a resposta perde-se.

**Reactivar NAT** no fim:

```routeros
/ip firewall nat enable 0
```

### 4.7 Respostas às perguntas da Exp 4

- **Rota estática no router comercial:** `/ip route add dst-address=<rede>/<máscara> gateway=<próximo-salto>`.
- **Caminhos com/sem ICMP redirect:** sem aceitar redirect, o pacote segue o gateway configurado (tux92 → RC → tux94), caminho subótimo; ao aceitar, o host instala rota de host directa (tux92 → tux94).
- **Configurar NAT:** regra `srcnat` com `action=masquerade` na interface externa (`out-interface=ether1`).
- **O que faz o NAT:** traduz endereços privados de origem para o IP público do router (e porta efémera), mantendo estado para desfazer nas respostas — permite que redes privadas acedam à Internet com 1 IP público.
- **Ping ao FTP sem NAT:** falha; as respostas têm destino `172.16.90.1` (privado), sem rota de retorno na rede do lab.

---

## EXPERIÊNCIA 5 — DNS

**Objectivo:** configurar resolução de nomes nos tuxes e observar as mensagens DNS.

### 5.1 Configurar DNS nos três tuxes

```bash
# Nos três tuxes (tux93, tux94, tux92) — executar em cada um:
echo -e "search netlab.fe.up.pt\nnameserver 10.227.20.3" > /etc/resolv.conf
cat /etc/resolv.conf    # confirmar
```

> Se `10.227.20.3` não for alcançável directamente, verifica que o tux tem rota via RC (o NAT garante saída para a rede de gestão).

### 5.2 Verificar resolução de nomes (passo 2)

```bash
ping -c 3 ftp.netlab.fe.up.pt    # deve resolver para 172.16.1.10
nslookup ftp.netlab.fe.up.pt     # alternativa
```

### 5.3 Capturar DNS (passo 3)

Inicia Wireshark em `tux93.eth1`, filtro `dns`. No **tux93**:

```bash
ping -c 3 www.google.com    # nome externo, ainda não em cache
```

**O que observar:**
- **DNS Query** (UDP, porto 53): pergunta pelo tipo **A** (IPv4) de `www.google.com`.
- **DNS Response**: retorna o(s) IP(s) e o TTL.
- Repetir o ping imediatamente pode não gerar nova query (cache local).

### 5.4 Respostas às perguntas da Exp 5

- **Configurar DNS:** editar `/etc/resolv.conf` com `nameserver <IP>` e `search <domínio>`.
- **Pacotes trocados:** Query e Response sobre **UDP/53**; a query transporta o nome e o tipo de registo (A); a resposta transporta os registos de recurso (nome → IP, TTL, etc.).

---

## EXPERIÊNCIA 6 — Ligações TCP (download FTP)

**Objectivo:** correr a aplicação `download` (Parte 1) e estudar TCP + FTP em modo passivo.

### 6.1 Compilar e lançar a aplicação (passos 1–2)

```bash
# tux93
git clone https://github.com/nunoleite144-design/RCOM2.git
cd RCOM2
make        # compila src/download.c → ./download
```

Inicia **Wireshark** em `tux93.eth1` (sem filtro, ou com `ftp or ftp-data or tcp`).

**Ficheiros disponíveis no servidor do lab** (credenciais: `rcom` / `rcom`):

```bash
# Ficheiro pequeno — bom para ver handshake e FTP control limpos
./download ftp://rcom:rcom@ftp.netlab.fe.up.pt/ftp/pipe.txt
./download ftp://rcom:rcom@ftp.netlab.fe.up.pt/ftp/teste.txt

# Ficheiro grande (~5 GB) — essencial para slow start / congestion avoidance
./download ftp://rcom:rcom@ftp.netlab.fe.up.pt/ftp/ubuntu-26.04-desktop-amd64.iso
```

> **Nota:** usa `pipe.txt` ou `teste.txt` para uma primeira verificação rápida; usa o `.iso` para observar TCP congestion control e o impacto do download simultâneo (passo 5).

### 6.2 Verificar o ficheiro (passo 3)

```bash
ls -l pipe.txt         # tamanho esperado
md5sum pipe.txt        # comparar integridade se houver referência
```

Guarda o log do Wireshark.

### 6.3 O que observar no Wireshark

**Ligação de controlo** (porto 21):
- 3-way handshake: `SYN` → `SYN,ACK` → `ACK`
- Comandos FTP em texto (filtro `ftp`): `USER`, `PASS`, `TYPE I`, `PASV`, `RETR`
- Resposta ao `PASV` (código `227`): contém o IP e porto para a ligação de dados

**Ligação de dados** (porto negociado no `PASV`):
- Novo 3-way handshake para a ligação de dados
- Transferência (filtro `ftp-data`): sequência de segmentos com ACKs cumulativos
- Fecho com `FIN`/`ACK`

**Passo 5 (simultâneo em tux92):** a meio da transferência em tux93, lança o mesmo download em tux92. Usa as estatísticas do Wireshark (menu *Statistics → TCP Stream Graph → Throughput*) para ver como o throughput de cada ligação evolui (partilha de largura de banda).

### 6.4 Respostas às perguntas da Exp 6

- **Quantas ligações TCP?** Duas: **controlo** (porto 21) e **dados** (porto negociado no PASV).
- **Modo passivo:** o cliente abre a ligação de dados para o IP:porto anunciado pelo servidor no `227 Entering Passive Mode` — funciona bem com NAT (o cliente inicia sempre).
- **Mecanismos TCP visíveis:** handshake SYN/SYN-ACK/ACK, ACKs cumulativos, janela de recepção (flow control), retransmissões (ARQ), slow start / congestion avoidance, terminação FIN.
- **Impacto do 2.º download:** o throughput de cada ligação reduz-se (partilha do mesmo bottleneck); os dois fluxos convergem para valores próximos (equidade TCP).

---

## Checklist de demonstração

Antes da demo, os professores anunciam IPs diferentes para as bridges — substitui o `9` (em todos os endereços) pelo valor anunciado.

- [ ] Bridges no switch: ether7+ether10 → bridge90 ; ether6+ether11+ether13 → bridge91
- [ ] tux94: `ip_forward=1`, `icmp_echo_ignore_broadcasts=0`, rotas directas em eth1 e eth2
- [ ] RC: IPs em ether1/ether2, rota para 172.16.90.0/24, default via .254, NAT masquerade activo
- [ ] tux93: rota para 172.16.91.0/24 e 172.16.1.0/24 via tux94
- [ ] tux94: rota para 172.16.1.0/24 via RC
- [ ] tux92: rota para 172.16.90.0/24 via tux94, rota para 172.16.1.0/24 via RC
- [ ] `ping 172.16.1.10` de tux93: funciona com NAT, falha sem NAT
- [ ] `/etc/resolv.conf` com `nameserver 10.227.20.3` nos 3 tuxes; `ping ftp.netlab.fe.up.pt` resolve
- [ ] `download` corre em tux93 e o ficheiro chega íntegro; logs guardados
- [ ] Demo com IPs novos: trocar Y=9 pelo valor anunciado em todo o endereçamento

---

## Referência rápida — comandos MikroTik mais usados

```routeros
# Ver bridges e portos
/interface bridge port print brief

# Adicionar/remover porto de bridge
/interface bridge port add bridge=bridge90 interface=ether7
/interface bridge port remove [find interface=ether7]

# Ver/adicionar endereços IP
/ip address print
/ip address add address=172.16.91.254/24 interface=ether2

# Ver/adicionar rotas
/ip route print
/ip route add dst-address=0.0.0.0/0 gateway=172.16.1.254

# Ver/gerir NAT
/ip firewall nat print
/ip firewall nat disable 0
/ip firewall nat enable 0

# Reset completo
/system reset-configuration
```

## Referência rápida — comandos Linux (tuxes)

```bash
ifconfig eth1 172.16.90.1/24          # configurar interface
route -n                               # ver tabela de rotas
route add -net 172.16.1.0/24 gw ...   # adicionar rota
route del -net 172.16.1.0/24          # remover rota
arp -d <ip>                            # apagar entrada ARP
ip -s neigh flush all                  # limpar toda a tabela ARP
sysctl net.ipv4.ip_forward=1           # activar forwarding
sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
sysctl net.ipv4.conf.all.accept_redirects=1
traceroute <ip>                        # caminho até ao destino
```
