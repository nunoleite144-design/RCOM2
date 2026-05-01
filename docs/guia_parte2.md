# Guia prático — Parte 2 do Lab 2 de Redes de Computadores

Este ficheiro é um guia de bolso para usar no laboratório da FEUP. A ideia é saberes exatamente o que fazer em cada experiência, que comandos usar, que resultados esperar e que informação guardar para o relatório.

> **Importante:** no guião, a letra `Y` representa o número do grupo/bancada. Como tens usado `tux92` e a rede `172.16.90.0/24`, este guia assume **Y = 9**. Se o teu grupo for outro, troca o `9` pelo número correto.

---

## 0. Antes de começares

### Máquinas principais

Para o grupo `Y = 9`, normalmente tens:

```text
tuxY2 = tux92
tuxY3 = tux93
tuxY4 = tux94
```

### Redes usadas

```text
Rede Y0: 172.16.90.0/24
Rede Y1: 172.16.91.0/24
Rede do laboratório/FTP: 172.16.1.0/24
```

### Endereços esperados ao longo do lab

```text
tuxY3 eth1  -> 172.16.90.1/24
tuxY4 eth1  -> 172.16.90.254/24
tuxY4 eth2  -> 172.16.91.253/24
tuxY2 eth1  -> 172.16.91.1/24
RC ether2   -> 172.16.91.254/24
RC ether1   -> 172.16.1.X/24   # confirmar X no laboratório/docente
FTP server  -> 172.16.1.10
DNS server  -> 10.227.20.3
```

### Descobrir nomes reais das interfaces

No guião aparece `eth1` e `eth2`, mas no computador pode aparecer algo como `enp2s0`, `if_e1`, `if_e2`, etc.

Usa:

```bash
ifconfig -a
```

ou:

```bash
ip link
```

Aponta sempre:

```text
Máquina:
Interface:
IP:
MAC:
```

### Regra de ouro para todas as experiências

Em cada experiência, guarda:

1. os comandos usados;
2. os IPs configurados;
3. os MAC addresses;
4. `route -n`;
5. `arp -a`;
6. screenshots/capturas do Wireshark;
7. explicação curta do que viste.

---

# Experiência 1 — Configurar uma rede IP simples

## Objetivo

Ligar dois tuxes na mesma rede IP, testar conectividade com `ping` e observar ARP + ICMP no Wireshark.

## Topologia

```text
172.16.90.0/24

+tuxY3 ---------------- switch ---------------- tuxY4+
  .1                                             .254
```

Para `Y = 9`:

```text
tux93 -> 172.16.90.1/24
tux94 -> 172.16.90.254/24
```

## Passo 1 — Ligar cabos

Ligar:

```text
E1 de tuxY3 ao switch
E1 de tuxY4 ao switch
```

## Passo 2 — Configurar IPs

No `tuxY3`:

```bash
sudo ifconfig eth1 up
sudo ifconfig eth1 172.16.90.1/24
```

No `tuxY4`:

```bash
sudo ifconfig eth1 up
sudo ifconfig eth1 172.16.90.254/24
```

Se a interface não se chamar `eth1`, substitui pelo nome real, por exemplo:

```bash
sudo ifconfig enp2s0 172.16.90.1/24
```

## Passo 3 — Apontar IP e MAC

Em cada máquina:

```bash
ifconfig eth1
```

Guardar:

```text
tuxY3 eth1:
IP  = 172.16.90.1
MAC = ...

tuxY4 eth1:
IP  = 172.16.90.254
MAC = ...
```

## Passo 4 — Ver rotas e ARP

No `tuxY3`:

```bash
route -n
arp -a
```

## Passo 5 — Limpar ARP antes do teste

No `tuxY3`:

```bash
sudo arp -d 172.16.90.254
```

Se disser que não existe entrada, tudo bem.

## Passo 6 — Capturar no Wireshark

No `tuxY3`:

```bash
sudo wireshark &
```

Selecionar a interface ligada ao switch, por exemplo `eth1`.

Filtro útil:

```text
arp || icmp
```

## Passo 7 — Fazer ping

No `tuxY3`:

```bash
ping -c 4 172.16.90.254
```

Esperado:

```text
4 packets transmitted, 4 received, 0% packet loss
```

## O que observar no Wireshark

Deves ver:

```text
ARP Request: Who has 172.16.90.254? Tell 172.16.90.1
ARP Reply:   172.16.90.254 is at <MAC do tuxY4>
ICMP Echo Request
ICMP Echo Reply
```

## Guardar para o relatório

Guardar ficheiro:

```text
captures/exp1_ping_tuxY3_tuxY4.pcapng
```

No relatório, explicar:

- ARP descobre o MAC correspondente a um IP na rede local;
- `ping` gera ICMP Echo Request e ICMP Echo Reply;
- Ethernet type `0x0806` indica ARP;
- Ethernet type `0x0800` indica IPv4;
- dentro do IP, protocolo `1` indica ICMP.

---

# Experiência 2 — Criar duas bridges no switch

## Objetivo

Separar a rede em dois domínios de broadcast usando duas bridges no switch Mikrotik.

## Topologia

```text
bridgeY0: tuxY3 + tuxY4 eth1
bridgeY1: tuxY2
```

Com `Y = 9`:

```text
bridge90 -> rede 172.16.90.0/24
bridge91 -> rede 172.16.91.0/24
```

## Passo 1 — Ligar/configurar tuxY2

No `tuxY2`:

```bash
sudo ifconfig eth1 up
sudo ifconfig eth1 172.16.91.1/24
ifconfig eth1
```

Apontar IP e MAC.

## Passo 2 — Entrar no switch Mikrotik

Abrir GTKterm numa das máquinas ligada por consola série:

```bash
sudo gtkterm &
```

Configuração típica:

```text
Baudrate: 115200
Username: admin
Password: vazio
```

## Passo 3 — Criar bridges

No terminal do Mikrotik:

```text
/interface bridge add name=bridge90
/interface bridge add name=bridge91
/interface bridge print
```

## Passo 4 — Tirar portas da bridge default e meter nas bridges certas

Primeiro ver portas:

```text
/interface bridge port print brief
```

Depois remover as portas necessárias da bridge default:

```text
/interface bridge port remove [find interface=etherX]
```

Adicionar portas à bridge90 e bridge91:

```text
/interface bridge port add bridge=bridge90 interface=etherX
/interface bridge port add bridge=bridge90 interface=etherY
/interface bridge port add bridge=bridge91 interface=etherZ
```

Substitui `etherX`, `etherY`, `etherZ` pelas portas reais onde estão ligados os tuxes.

Confirmar:

```text
/interface bridge port print brief
```

## Passo 5 — Testar pings

No `tuxY3`:

```bash
ping -c 4 172.16.90.254     # tuxY4, deve funcionar
ping -c 4 172.16.91.1       # tuxY2, nesta fase normalmente não deve funcionar sem router
```

## Passo 6 — Testar broadcast

Capturar em `tuxY2`, `tuxY3` e `tuxY4` se possível.

No `tuxY3`:

```bash
ping -b 172.16.90.255
```

Pode ser necessário:

```bash
ping -b -c 4 172.16.90.255
```

Depois, no `tuxY2`:

```bash
ping -b -c 4 172.16.91.255
```

## O que observar

- Broadcast enviado na bridge90 só deve aparecer em máquinas da bridge90.
- Broadcast enviado na bridge91 só deve aparecer em máquinas da bridge91.
- Assim provas que há dois domínios de broadcast.

## Guardar para o relatório

```text
captures/exp2_ping_tuxY3_tuxY4.pcapng
captures/exp2_broadcast_bridge90.pcapng
captures/exp2_broadcast_bridge91.pcapng
```

Explicar:

- o que é uma bridge;
- o que é um domínio de broadcast;
- porque é que broadcasts de uma bridge não passam para a outra.

---

# Experiência 3 — Transformar tuxY4 num router Linux

## Objetivo

Fazer o `tuxY4` encaminhar pacotes entre duas redes:

```text
172.16.90.0/24 <-> 172.16.91.0/24
```

## Topologia

```text
tuxY3                 tuxY4/router                  tuxY2
172.16.90.1      172.16.90.254 | 172.16.91.253      172.16.91.1
```

## Passo 1 — Configurar interfaces

No `tuxY3`:

```bash
sudo ifconfig eth1 172.16.90.1/24
```

No `tuxY4`:

```bash
sudo ifconfig eth1 172.16.90.254/24
sudo ifconfig eth2 172.16.91.253/24
```

No `tuxY2`:

```bash
sudo ifconfig eth1 172.16.91.1/24
```

## Passo 2 — Ativar encaminhamento no tuxY4

No `tuxY4`:

```bash
sudo sysctl net.ipv4.ip_forward=1
sudo sysctl net.ipv4.icmp_echo_ignore_broadcasts=0
```

Confirmar:

```bash
cat /proc/sys/net/ipv4/ip_forward
```

Deve dar:

```text
1
```

## Passo 3 — Configurar rotas

No `tuxY3`, para chegar à rede Y1:

```bash
sudo route add -net 172.16.91.0/24 gw 172.16.90.254
```

No `tuxY2`, para chegar à rede Y0:

```bash
sudo route add -net 172.16.90.0/24 gw 172.16.91.253
```

Verificar nas três máquinas:

```bash
route -n
```

## Passo 4 — Testar pings

No `tuxY3`:

```bash
ping -c 4 172.16.90.254    # interface eth1 do tuxY4
ping -c 4 172.16.91.253    # interface eth2 do tuxY4
ping -c 4 172.16.91.1      # tuxY2
```

O último é o teste mais importante: prova que o routing funciona.

## Passo 5 — Capturas

No `tuxY4`, abrir duas capturas Wireshark, uma em cada interface:

```bash
sudo wireshark &
```

Capturar em:

```text
eth1: lado 172.16.90.0/24
eth2: lado 172.16.91.0/24
```

Limpar ARP antes:

```bash
sudo ip neigh flush all
```

Ou, se necessário, apagar entradas específicas com:

```bash
sudo arp -d <ip>
```

No `tuxY3`:

```bash
ping -c 4 172.16.91.1
```

## O que observar

No IP:

```text
Origem: 172.16.90.1
Destino: 172.16.91.1
```

O IP origem/destino mantém-se fim-a-fim.

No Ethernet/MAC:

```text
Na rede 172.16.90.0/24:
MAC origem = tuxY3
MAC destino = tuxY4 eth1

Na rede 172.16.91.0/24:
MAC origem = tuxY4 eth2
MAC destino = tuxY2
```

Isto é essencial: **o router muda os MACs, mas não muda os IPs**.

---

# Experiência 4 — Router comercial e NAT

## Objetivo

Adicionar o router comercial `RC`, configurar rotas, estudar caminhos dos pacotes, ICMP redirect e NAT.

## Topologia geral

```text
172.16.90.0/24       172.16.91.0/24          172.16.1.0/24

tuxY3 ---- tuxY4/router ---- tuxY2/RC ---- rede lab/FTP
 .1        .254 | .253       .1 / .254       FTP 172.16.1.10
```

## Passo 1 — Ligar RC

Ligações típicas:

```text
RC ether1 -> rede do laboratório 172.16.1.0/24
RC ether2 -> bridgeY1, rede 172.16.91.0/24
```

## Passo 2 — Configurar IPs no RC

Entrar por consola série no router Mikrotik.

Exemplo para a interface ligada à rede do laboratório:

```text
/ip address add address=172.16.1.X/24 interface=ether1
```

Confirmar com o professor qual é o `X` correto.

Interface ligada à bridgeY1:

```text
/ip address add address=172.16.91.254/24 interface=ether2
/ip address print
```

## Passo 3 — Configurar rotas nos tuxes

No `tuxY3`:

```bash
sudo route add -net 172.16.91.0/24 gw 172.16.90.254
sudo route add -net 172.16.1.0/24 gw 172.16.90.254
```

No `tuxY4`:

```bash
sudo route add -net 172.16.1.0/24 gw 172.16.91.254
```

No `tuxY2`:

```bash
sudo route add -net 172.16.90.0/24 gw 172.16.91.253
sudo route add -net 172.16.1.0/24 gw 172.16.91.254
```

No RC, rota para voltar à rede Y0:

```text
/ip route add dst-address=172.16.90.0/24 gateway=172.16.91.253
/ip route print
```

## Passo 4 — Testar conectividade

No `tuxY3`:

```bash
ping -c 4 172.16.90.254    # tuxY4 lado Y0
ping -c 4 172.16.91.253    # tuxY4 lado Y1
ping -c 4 172.16.91.1      # tuxY2
ping -c 4 172.16.91.254    # RC lado Y1
ping -c 4 172.16.1.10      # FTP server
```

## Passo 5 — Estudar ICMP redirect

No `tuxY2`, desativar redirects:

```bash
sudo sysctl net.ipv4.conf.eth1.accept_redirects=0
sudo sysctl net.ipv4.conf.all.accept_redirects=0
```

Mudar a rota para a rede Y0 passar pelo RC em vez de passar pelo tuxY4:

```bash
sudo route del -net 172.16.90.0/24
sudo route add -net 172.16.90.0/24 gw 172.16.91.254
```

No `tuxY2`:

```bash
ping -c 4 172.16.90.1
traceroute 172.16.90.1
```

Depois restaurar a rota correta via tuxY4:

```bash
sudo route del -net 172.16.90.0/24
sudo route add -net 172.16.90.0/24 gw 172.16.91.253
traceroute 172.16.90.1
```

Para observar redirects, voltar a permitir redirects e criar situação em que o gateway inicial não é o caminho ideal:

```bash
sudo sysctl net.ipv4.conf.eth1.accept_redirects=1
sudo sysctl net.ipv4.conf.all.accept_redirects=1
```

Capturar no Wireshark e procurar mensagens ICMP Redirect.

## Passo 6 — NAT

No RC, ver regras NAT:

```text
/ip firewall nat print
```

Ativar NAT, se necessário:

```text
/ip firewall nat add chain=srcnat action=masquerade out-interface=ether1
```

Testar no `tuxY3`:

```bash
ping -c 4 172.16.1.10
```

Depois desativar NAT:

```text
/ip firewall nat disable 0
```

Testar novamente:

```bash
ping -c 4 172.16.1.10
```

## O que observar

Com NAT, o servidor FTP/rede externa vê os pacotes como se viessem do endereço do RC na rede `172.16.1.0/24`.

Sem NAT, o servidor vê origem privada/laboratorial como `172.16.90.1`, e a resposta pode não saber voltar corretamente.

---

# Experiência 5 — DNS

## Objetivo

Configurar DNS nos tuxes e observar pacotes DNS no Wireshark.

## Passo 1 — Configurar DNS

Em `tuxY3`, `tuxY4` e `tuxY2`:

```bash
sudo nano /etc/resolv.conf
```

Colocar:

```text
nameserver 10.227.20.3
```

Alternativa rápida:

```bash
echo "nameserver 10.227.20.3" | sudo tee /etc/resolv.conf
```

## Passo 2 — Testar nomes

```bash
ping -c 4 ftp.netlab.fe.up.pt
```

ou:

```bash
host ftp.netlab.fe.up.pt
```

## Passo 3 — Capturar DNS

No Wireshark, filtro:

```text
dns
```

Fazer:

```bash
ping -c 4 ftp.netlab.fe.up.pt
```

## O que observar

Deves ver:

```text
DNS Query: quem é ftp.netlab.fe.up.pt?
DNS Response: endereço IP correspondente
```

DNS usa normalmente UDP porta 53.

Guardar:

```text
captures/exp5_dns.pcapng
```

---

# Experiência 6 — Ligações TCP e aplicação FTP

## Objetivo

Correr a tua aplicação `download`, capturar no Wireshark e analisar as ligações TCP e FTP.

## Passo 1 — Obter o código no tuxY3

Se tiveres acesso ao GitHub no laboratório:

```bash
git clone https://github.com/nunoleite144-design/RCOM2.git
cd RCOM2
```

Se não der por ser privado, leva o código numa pen ou faz download manual.

## Passo 2 — Compilar

```bash
make
```

Se não houver `make`:

```bash
gcc -Wall -Wextra -pedantic -std=c11 src/download.c -o download
```

## Passo 3 — Capturar no Wireshark

No `tuxY3`:

```bash
sudo wireshark &
```

Filtro útil:

```text
tcp || ftp
```

Ou, depois de saberes o IP do servidor:

```text
ip.addr == 172.16.1.10
```

## Passo 4 — Executar a aplicação

Usar o URL indicado pelo professor. Exemplo geral:

```bash
./download ftp://ftp.netlab.fe.up.pt/pub/...
```

Confirmar que o ficheiro apareceu:

```bash
ls -lh
```

## Passo 5 — Guardar captura

```text
captures/exp6_ftp_download.pcapng
```

## O que observar

A tua aplicação deve abrir duas ligações TCP:

1. ligação de controlo FTP, para a porta 21;
2. ligação de dados FTP, para o porto recebido na resposta `PASV`.

Na ligação de controlo deves ver comandos como:

```text
USER
PASS
TYPE I
PASV
RETR
QUIT
```

Nas ligações TCP deves identificar:

```text
Estabelecimento: SYN, SYN-ACK, ACK
Transferência: segmentos TCP com dados e ACKs
Terminação: FIN/ACK ou equivalente
```

## Passo 6 — Segundo download em simultâneo

O guião pede repetir o download no `tuxY3` e, a meio da transferência, iniciar outro download no `tuxY2`.

No `tuxY3`:

```bash
./download ftp://ftp.netlab.fe.up.pt/pub/ficheiro_grande
```

Enquanto está a descarregar, no `tuxY2`:

```bash
./download ftp://ftp.netlab.fe.up.pt/pub/ficheiro_grande
```

Usar ficheiro suficientemente grande para dar tempo de iniciar o segundo download.

No Wireshark, usar:

```text
Statistics -> TCP Stream Graphs
Statistics -> I/O Graphs
```

Objetivo: perceber se o throughput da primeira ligação muda quando aparece a segunda.

---

# Checklist final para levar para o relatório

Para cada experiência, preencher isto:

```text
Experiência nº:
Objetivo:
Topologia:
Comandos usados:
IPs configurados:
MACs observados:
Rotas relevantes:
Captura Wireshark guardada:
Pacotes importantes observados:
Conclusão curta:
```

## Capturas mínimas recomendadas

```text
exp1_ping_arp_icmp.pcapng
exp2_bridges_broadcast.pcapng
exp3_router_linux_tuxY3_to_tuxY2.pcapng
exp4_nat_on_off.pcapng
exp5_dns.pcapng
exp6_ftp_download.pcapng
```

## Comandos que vais usar muitas vezes

```bash
ifconfig -a
ifconfig eth1
sudo ifconfig eth1 up
sudo ifconfig eth1 172.16.90.1/24
route -n
arp -a
sudo arp -d <ip>
sudo ip neigh flush all
ping -c 4 <ip>
ping -b -c 4 <broadcast>
traceroute <ip>
sudo wireshark &
```

## Filtros Wireshark úteis

```text
arp
icmp
arp || icmp
dns
tcp
ftp
tcp.port == 21
ip.addr == <ip>
```

---

# Mini-resumo mental

```text
Exp 1: duas máquinas na mesma rede -> ARP + ICMP
Exp 2: duas bridges -> domínios de broadcast separados
Exp 3: tuxY4 vira router -> rotas entre Y0 e Y1
Exp 4: router comercial + NAT -> caminhos, redirects e acesso ao FTP server
Exp 5: DNS -> nomes viram IPs
Exp 6: aplicação download -> duas ligações TCP: controlo FTP + dados FTP
```
