# app.py
from flask import Flask, render_template, send_file, Response, abort, jsonify, request, url_for, redirect
from sqlalchemy.sql import text
# Para o upload de arquivos
from werkzeug.utils import secure_filename
# Para a autenticação
from flask_httpauth import HTTPBasicAuth
from werkzeug.security import generate_password_hash, check_password_hash
# Experiments Models
from Model import *

auth = HTTPBasicAuth()

users = {
    "admin": generate_password_hash("letmein")
}

@auth.verify_password
def verify_password(username, password):
    if username in users and \
            check_password_hash(users.get(username), password):
        return username

app = Flask(__name__, template_folder="templates")
app.config['MAX_CONTENT_LENGTH'] = 1024 * 1024
app.config['UPLOAD_EXTENSIONS'] = ['.sql']
app.config['UPLOAD_PATH'] = 'uploads/'

@app.route('/')
def hello():
    exp = db.query(Experiment).all()
    qtd = len(exp)
    return render_template("index.html", count=qtd, experiments=exp)

@app.route('/experiment/<id>')
def detailLocation(id):
    exp = db.query(Experiment).filter_by(id=id).first()
    return render_template("expDetail.html", exp=exp)

@app.route('/experiment/run/<id>')
@auth.login_required
def runExp(id):
    db.query(Experiment).filter_by(id=id).first().run()
    return render_template("run.html", user=auth.current_user())

if __name__ == '__main__':
    app.run(host="0.0.0.0", debug=True)