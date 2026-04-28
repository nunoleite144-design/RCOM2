# GRP Monitor (Flask + SQLite)

Aplicação web simples para monitorização de Guias de Reparação (GRP) de equipamentos hospitalares.

## Funcionalidades

- Criar, editar e listar GRP
- Pesquisa textual (número GRP, equipamento, serviço, nº série)
- Estados normalizados em catálogo (`aberta`, `em análise`, `aguarda peças`, `em reparação`, `fechada`)
- Alertas de prazo ultrapassado
- Dashboard com indicadores básicos
- Exportação para Excel (`.xlsx`)

## Requisitos

- Python 3.10+

## Como executar

```bash
cd grp_monitor
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python app.py
```

A aplicação ficará disponível em `http://127.0.0.1:5000`.

## Estrutura

- `app.py`: configuração da app, modelos, rotas e lógica principal
- `templates/`: páginas HTML (dashboard, listagem e formulário)
- `static/style.css`: estilo básico da interface

## Notas de implementação

- A base de dados SQLite (`grp_monitor.db`) é criada automaticamente.
- Os estados padrão são semeados na primeira execução.
- O destaque de atraso considera GRP com prazo anterior à data atual e estado diferente de `fechada`.
