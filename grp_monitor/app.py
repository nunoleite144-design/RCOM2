"""Aplicação Flask para monitorização de Guias de Reparação (GRP).

Funcionalidades principais:
- CRUD de GRP (criar, listar, editar e estado)
- Pesquisa por número GRP, equipamento e serviço
- Estados normalizados num catálogo próprio
- Alertas de prazo ultrapassado (overdue)
- Dashboard simples com métricas e distribuição por estado
- Exportação para Excel (.xlsx)
"""

from __future__ import annotations

from datetime import date, datetime
from io import BytesIO

import pandas as pd
from flask import Flask, flash, redirect, render_template, request, send_file, url_for
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy import func

app = Flask(__name__)
app.config["SECRET_KEY"] = "grp-secret-key-dev"
app.config["SQLALCHEMY_DATABASE_URI"] = "sqlite:///grp_monitor.db"
app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False

db = SQLAlchemy(app)


class EstadoGRP(db.Model):
    """Catálogo normalizado de estados de uma GRP."""

    __tablename__ = "estados_grp"

    id = db.Column(db.Integer, primary_key=True)
    codigo = db.Column(db.String(30), unique=True, nullable=False)
    nome = db.Column(db.String(80), nullable=False)
    ordem = db.Column(db.Integer, nullable=False, default=0)

    def __repr__(self) -> str:
        return f"EstadoGRP(codigo={self.codigo!r}, nome={self.nome!r})"


class GRP(db.Model):
    """Representa uma Guia de Reparação de equipamento hospitalar."""

    __tablename__ = "grps"

    id = db.Column(db.Integer, primary_key=True)
    numero_grp = db.Column(db.String(40), unique=True, nullable=False)
    equipamento = db.Column(db.String(120), nullable=False)
    numero_serie = db.Column(db.String(120), nullable=True)
    servico = db.Column(db.String(120), nullable=False)
    descricao_avaría = db.Column(db.Text, nullable=False)

    data_abertura = db.Column(db.Date, nullable=False)
    prazo_reparacao = db.Column(db.Date, nullable=False)
    data_fecho = db.Column(db.Date, nullable=True)

    estado_id = db.Column(db.Integer, db.ForeignKey("estados_grp.id"), nullable=False)
    estado = db.relationship("EstadoGRP", lazy="joined")

    tecnico_responsavel = db.Column(db.String(120), nullable=True)
    fornecedor = db.Column(db.String(120), nullable=True)
    notas = db.Column(db.Text, nullable=True)

    @property
    def em_atraso(self) -> bool:
        """Sinaliza atraso quando a data de prazo já passou e a GRP não está fechada."""
        hoje = date.today()
        return self.prazo_reparacao < hoje and self.estado.codigo != "fechada"


ESTADOS_DEFAULT = [
    ("aberta", "Aberta", 1),
    ("em_analise", "Em análise", 2),
    ("aguarda_pecas", "Aguarda peças", 3),
    ("em_reparacao", "Em reparação", 4),
    ("fechada", "Fechada", 5),
]


def seed_estados() -> None:
    """Cria estados padrão se ainda não existirem na base de dados."""
    for codigo, nome, ordem in ESTADOS_DEFAULT:
        if not EstadoGRP.query.filter_by(codigo=codigo).first():
            db.session.add(EstadoGRP(codigo=codigo, nome=nome, ordem=ordem))
    db.session.commit()


def parse_date(valor: str | None) -> date | None:
    """Converte string YYYY-MM-DD em date."""
    if not valor:
        return None
    return datetime.strptime(valor, "%Y-%m-%d").date()


@app.route("/")
def dashboard():
    """Dashboard com métricas agregadas e GRPs em atraso."""
    total = GRP.query.count()
    abertas = (
        GRP.query.join(EstadoGRP)
        .filter(EstadoGRP.codigo != "fechada")
        .count()
    )
    fechadas = (
        GRP.query.join(EstadoGRP)
        .filter(EstadoGRP.codigo == "fechada")
        .count()
    )

    hoje = date.today()
    em_atraso = (
        GRP.query.join(EstadoGRP)
        .filter(GRP.prazo_reparacao < hoje, EstadoGRP.codigo != "fechada")
        .order_by(GRP.prazo_reparacao.asc())
        .all()
    )

    por_estado = (
        db.session.query(EstadoGRP.nome, func.count(GRP.id))
        .outerjoin(GRP, GRP.estado_id == EstadoGRP.id)
        .group_by(EstadoGRP.nome, EstadoGRP.ordem)
        .order_by(EstadoGRP.ordem.asc())
        .all()
    )

    return render_template(
        "dashboard.html",
        total=total,
        abertas=abertas,
        fechadas=fechadas,
        em_atraso=em_atraso,
        por_estado=por_estado,
    )


@app.route("/grps")
def listar_grps():
    """Lista GRPs e permite pesquisar por campos principais."""
    termo = request.args.get("q", "").strip()

    query = GRP.query.join(EstadoGRP)
    if termo:
        like = f"%{termo}%"
        query = query.filter(
            db.or_(
                GRP.numero_grp.ilike(like),
                GRP.equipamento.ilike(like),
                GRP.servico.ilike(like),
                GRP.numero_serie.ilike(like),
            )
        )

    grps = query.order_by(GRP.data_abertura.desc()).all()
    return render_template("grp_list.html", grps=grps, termo=termo)


@app.route("/grps/novo", methods=["GET", "POST"])
def criar_grp():
    """Criação de uma nova GRP."""
    estados = EstadoGRP.query.order_by(EstadoGRP.ordem.asc()).all()

    if request.method == "POST":
        try:
            grp = GRP(
                numero_grp=request.form["numero_grp"].strip(),
                equipamento=request.form["equipamento"].strip(),
                numero_serie=request.form.get("numero_serie", "").strip() or None,
                servico=request.form["servico"].strip(),
                descricao_avaría=request.form["descricao_avaria"].strip(),
                data_abertura=parse_date(request.form.get("data_abertura")),
                prazo_reparacao=parse_date(request.form.get("prazo_reparacao")),
                data_fecho=parse_date(request.form.get("data_fecho")),
                estado_id=int(request.form["estado_id"]),
                tecnico_responsavel=request.form.get("tecnico_responsavel", "").strip() or None,
                fornecedor=request.form.get("fornecedor", "").strip() or None,
                notas=request.form.get("notas", "").strip() or None,
            )
            db.session.add(grp)
            db.session.commit()
            flash("GRP criada com sucesso.", "success")
            return redirect(url_for("listar_grps"))
        except Exception as exc:  # feedback amigável ao utilizador
            db.session.rollback()
            flash(f"Erro ao criar GRP: {exc}", "danger")

    return render_template("grp_form.html", grp=None, estados=estados)


@app.route("/grps/<int:grp_id>/editar", methods=["GET", "POST"])
def editar_grp(grp_id: int):
    """Edição de uma GRP existente."""
    grp = GRP.query.get_or_404(grp_id)
    estados = EstadoGRP.query.order_by(EstadoGRP.ordem.asc()).all()

    if request.method == "POST":
        try:
            grp.numero_grp = request.form["numero_grp"].strip()
            grp.equipamento = request.form["equipamento"].strip()
            grp.numero_serie = request.form.get("numero_serie", "").strip() or None
            grp.servico = request.form["servico"].strip()
            grp.descricao_avaría = request.form["descricao_avaria"].strip()
            grp.data_abertura = parse_date(request.form.get("data_abertura"))
            grp.prazo_reparacao = parse_date(request.form.get("prazo_reparacao"))
            grp.data_fecho = parse_date(request.form.get("data_fecho"))
            grp.estado_id = int(request.form["estado_id"])
            grp.tecnico_responsavel = request.form.get("tecnico_responsavel", "").strip() or None
            grp.fornecedor = request.form.get("fornecedor", "").strip() or None
            grp.notas = request.form.get("notas", "").strip() or None

            db.session.commit()
            flash("GRP atualizada com sucesso.", "success")
            return redirect(url_for("listar_grps"))
        except Exception as exc:
            db.session.rollback()
            flash(f"Erro ao editar GRP: {exc}", "danger")

    return render_template("grp_form.html", grp=grp, estados=estados)


@app.route("/grps/exportar")
def exportar_excel():
    """Exporta a lista de GRPs para um ficheiro Excel."""
    grps = (
        GRP.query.join(EstadoGRP)
        .order_by(GRP.data_abertura.desc())
        .all()
    )

    rows = []
    for g in grps:
        rows.append(
            {
                "Número GRP": g.numero_grp,
                "Equipamento": g.equipamento,
                "Nº Série": g.numero_serie,
                "Serviço": g.servico,
                "Estado": g.estado.nome,
                "Data Abertura": g.data_abertura.isoformat() if g.data_abertura else "",
                "Prazo": g.prazo_reparacao.isoformat() if g.prazo_reparacao else "",
                "Data Fecho": g.data_fecho.isoformat() if g.data_fecho else "",
                "Técnico": g.tecnico_responsavel,
                "Fornecedor": g.fornecedor,
                "Em Atraso": "Sim" if g.em_atraso else "Não",
            }
        )

    df = pd.DataFrame(rows)
    output = BytesIO()
    with pd.ExcelWriter(output, engine="openpyxl") as writer:
        df.to_excel(writer, sheet_name="GRPs", index=False)

    output.seek(0)
    return send_file(
        output,
        as_attachment=True,
        download_name="grps.xlsx",
        mimetype="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    )


@app.cli.command("init-db")
def init_db_command():
    """Comando de utilidade: cria tabelas e estados base."""
    db.create_all()
    seed_estados()
    print("Base de dados inicializada com sucesso.")


with app.app_context():
    db.create_all()
    seed_estados()


if __name__ == "__main__":
    app.run(debug=True)
